// @file rcs-rx04-03.c
// @date 2021.09.03

// @info ultrasound-rangefinder receiver
// @info ADC capture tx pulse over defined threshold, define a reference pulse flight time
// @info by measuring a first known pulse at a known distance (1ft). Use reference pulse
// @info to determine distance of subsequent pulses.  Display distance to nearest ft
// @info in binary on 4 LEDs.

// Copyright 2022 RC Schuler. All rights reserved.
// Use of this source code is governed by a GNU V3
// license that can be found in the LICENSE file.

// @build cd build; make -j4 ;# init with mkdir build; cd build; cmake ..
// @build xfr firmware: boot w/boot_sel, cp -rp <outfile>.uf2 /media/pi/RPI_RP2

// @date 2021.09.03 fork from rcs-rx04-01
// @date 2021.09.10 2pm: first commit after tx01-01 pwm bug corrected.  previous  rcs-tx04-01 commits contain pwm bug. 
//                       added correction for measured tx01->rx04 timing skew of -18.07
// @date 2021.09.11 added led binary output of distance
// @date 2021.09.13 moved generic functions/globals to ../rcs-common/rcs-utils-01.*; added G_GP15_MISS missed pulse indicator
// @date 2021.10.18 timer kickoff delay set to TX_PERIOD/3 (was fixed 2000 assuming 4000 tx period); adjusted TX_RX_SKEW;
//                  reduced TX_PERIOD from 4000 to 2000 (tx and rx); moved G_bFlightTimeBusySemaphore reset (false) inside get_flight_time()
//                  added ( uLocalFlightTimeSemaphore != G_uFlightTimeSemaphore ) test before processing valid pulse;
//                  changed busy_wait_ms() to sleep_ms() in infinte while loop

// @require raspi pico (2020)
// @require hardware: subject: "rx-04 layout design 2021.08.06" https://mail.google.com/mail/u/1/#label/design...
// @require quiet 40Khz ambient audio environment
// @require adc input GP26_ADC0 (pin 31); pin 28 is gnd
// @require usb connection to raspi host; internal pico LED; minicom
// @require minicom -b 115200 -o -D /dev/ttyACM0 ;# monitor serial i/o
//          c-a z -> menu, c-a x -> exit; -C or --capturefile=~/Downloads/mc.dat

// @info uses globals G_uBuf* for i/o; 1024 max chars (if char input in use)
// @info "1st 8 gpio boot 50K pull up, remainder pull down; all programmable"
// @info (/ 1.0 40e3) 25us pulse period
// @info (/ 1.0 500e3) 2e-06 2us adc sample interval 

// @info see s*r*.txt for dev info
// #define MC // mincom support; enables stdio; gates program start prior to mc connection; see also ../rcs-common sources
#define TX_PERIOD 2000 // mus match rcs-tx01-xx.c (original was 4000)
#define MISSED_PULSE -9999
#define TX_RX_SKEW -10 // --20211018-- was TX_RX_SKEW -16

#include "pico/stdlib.h"
#include <stdio.h>
#include <stdlib.h>
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include <inttypes.h> // printf("%" PRIu64/d64/x64 "\n", t)
#include "../rcs-common/rcs-utils-01.h" // global extern: G_LED_PIN, G_uBuf, G_uBufCt

// globals
// - repeating timer params; gloals avoid passed args
uint16_t G_uAdcTriggerPos; // positive trigger: baseline_avg+threshold
uint16_t G_uAdcTriggerNeg; // negative trigger: baseline_avg-threshold
volatile uint64_t G_uFlightTimeSemaphore = 0;     // updated by main and hw timer callback
volatile bool G_bFlightTimeBusySemaphore = false; // updated by hw timer callback
// - main sleep loop reporting
volatile int64_t G_FlightTimeReport;
// gpio binary led distance display 0-15 -> (0000 - 1111)
const uint G_GP2_BIT0    =  2; // pin 4
const uint G_GP3_BIT1    =  3; // pin 5
const uint G_GP4_BIT2    =  4; // pin 6
const uint G_GP5_BIT3    =  5; // pin 7
// gpio general
const uint G_GP15_MISS   = 15; // pin 20 // missed pulse

// functions
void gpio_out_bit(uint GPX, uint v) {
  // configure GPX as output, set to LSB of v
  v = v & 0x01; // isolate LSB
  gpio_init(GPX);
  gpio_set_dir(GPX, GPIO_OUT);
  gpio_put(GPX, v);
}

void gpio_led_float_to_bin4(float f) {
  // round float f to int and display as binary on 4 led display
  uint uValue = f+0.5;
  uint bit0 = uValue & 1;
  uint bit1 = (uValue >> 1) & 1;
  uint bit2 = (uValue >> 2) & 1;
  uint bit3 = (uValue >> 3) & 1;
  #if defined(MC)
  // printf("--debug-- uValue: %d  %u%u%u%u\n", uValue, bit3, bit2, bit1, bit0);
  #endif
  // output to 4 bit led display
  gpio_out_bit(G_GP5_BIT3, bit3);
  gpio_out_bit(G_GP4_BIT2, bit2);
  gpio_out_bit(G_GP3_BIT1, bit1);
  gpio_out_bit(G_GP2_BIT0, bit0);
}

bool wait_for_pulse() {
  // poll forever; adc_read() until threshold values are exceeded, or timeout received
  // via global semaphore; used to detect first tx pulse and kick off the
  // flight time measurement process;  returns true on pulse, or false on timeout.
  // note the visual indicator flash_led_16hz() could become timing issue in some use cases.
  // @todo the trigger condition can become a function if call overhead is not an issue
  uint64_t uLocalFlightTimeSemaphore = G_uFlightTimeSemaphore; // potential timeout
  while ( uLocalFlightTimeSemaphore == G_uFlightTimeSemaphore ) {
    uint16_t adc_sample = adc_read();
    if ((adc_sample >= G_uAdcTriggerPos) || (adc_sample <= G_uAdcTriggerNeg)) {
      // pulse received
      flash_led_16hz();
      return true;
    }
  } // end while ( uLocalFlightTimeSemaphore == G_uFlightTimeSemaphore )
  return false; // timeout condition
} // end bool wait_for_pulse() 
    
int64_t get_flight_time() {
  // determine flight time of latest pulse by comparing to a reference flight time determined
  // by the first get_flight_time call. this function is called at a fixed timer interval by
  // the hardware timer (TX_PERIOD). flight time of the current pulse is returned. on the first
  // call to this function, the reference time is determined and a calculated zero is returned.
  // at function start, a local copy of a global semephore is set. if the global semaphore
  // changes before a pulse is captured, the function is returned as MISSED_PULSE.
  if ( G_bFlightTimeBusySemaphore ) { // another instance of get_flight_time() is running
    gpio_put(G_GP15_MISS, 1); 
    return MISSED_PULSE;
  }

  G_bFlightTimeBusySemaphore = true;        // notify function is active
  uint64_t uFuncStartTime = time_us_64();   // function start time
  uint64_t uFuncTimeoutTime = uFuncStartTime + TX_PERIOD*1E3;
  static bool bFirstCall = true;            // determine uRefTime on first call
  static uint64_t uRefTime;                 // flight time  of reference distance
  int64_t iDeltaFlightTime = 0LL;           // difference of current flight time and reference
  uint16_t uAdcSample;

  // wait for pulse or the next hw timer callback (timeout)
  uint64_t uPulseTime = 0LL;                // time of received pulse
  uint64_t uLocalFlightTimeSemaphore = G_uFlightTimeSemaphore; // detect missed pulses
  while ( uLocalFlightTimeSemaphore == G_uFlightTimeSemaphore ) {
    uAdcSample = adc_read();
    if ((uAdcSample >= G_uAdcTriggerPos) || (uAdcSample <= G_uAdcTriggerNeg)) {
	uPulseTime = time_us_64(); // pulse received
	break;
    }
    if ( time_us_64() >= uFuncTimeoutTime ) { // timeout
      G_bFlightTimeBusySemaphore = false;
      gpio_put(G_GP15_MISS, 1); 
      return MISSED_PULSE;
    }
  } // end while ( uLocalFlightTimeSemaphore == G_uFlightTimeSemaphore ) 
  if ( uLocalFlightTimeSemaphore != G_uFlightTimeSemaphore ) return MISSED_PULSE;
      
  // process the valid pulse
  gpio_put(G_GP15_MISS, 0); // clear missed pulse indicator
  if ( bFirstCall ) {
    uRefTime = uPulseTime-uFuncStartTime;
    bFirstCall = false;
    gpio_led_float_to_bin4(15.0); // flash 4 bit display to indicate capture
    flash_led_16hz();
    gpio_led_float_to_bin4(0.0);
    flash_led_16hz();
    gpio_led_float_to_bin4(15.0); // flash 4 bit display to indicate capture
    flash_led_16hz();
    gpio_led_float_to_bin4(0.0);
    // flash_led_16hz(); --20211018-- removed; potential timing error
  } 
  iDeltaFlightTime = (uPulseTime-uFuncStartTime) - uRefTime; // calculates to zero on bFirstCall
  G_bFlightTimeBusySemaphore = false; 
  return iDeltaFlightTime;
} // end int64_t get_flight_time() 

bool repeating_timer_callback(struct repeating_timer *t) {
  static uint64_t uPulseCount = 0LL;
  G_uFlightTimeSemaphore = time_us_64(); 
  G_FlightTimeReport = get_flight_time()-(uPulseCount++ * TX_RX_SKEW); // tweek tx/rx clock skew
  // --20211018-- G_bFlightTimeBusySemaphore = false; // set true at start of get_flight_time(); moved into get_flight_time()
  flash_led_16hz();
  // approx 'fDistance' based on 1ms/ft sound in air, 1.0 is initial calibration distance
  float fDistance = (float)G_FlightTimeReport/1000.0 + 1.0; 
  gpio_led_float_to_bin4(fDistance);
  #if defined(MC)
  printf("Distance:\t%3.0f ft\t%" PRId64 " uSec     \n", fDistance,G_FlightTimeReport);
  #endif
  return true;
}

int main() {
#ifndef PICO_DEFAULT_LED_PIN
#warning Error: requires board with integrated LED defined as PICO_DEFAULT_LED_PIN
#else
  // fixed vars
  const float fAdcThreshold = 0.05; // fAdcThreshold over average triggers a capture

  // gpio resources (also see globals)
  const uint GP26_ADC0   = 26; // pin 31
  // gpio are used for transistor biasing
  const uint GP21_IREF   = 21; // iref mirror master pullup
  const uint GP19_INP    = 19; // diff pair in plus pull up
  const uint GP12_INM    = 12; // diff pair in minus pull up
  const uint GP18_LOADP  = 18; // diff pair plus leg load pull up
  const uint GP13_LOADM  = 13; // diff pair minus leg load  pull up
  const uint GP20_INPLO  = 20; // diff pair in plus low res pull down
  const uint GP11_INMLO  = 11; // diff pair in minus low res pull down
  // rx vars
  const uint uNsettle = 128;   // 1/2 length of initial ADC settling capture
  // state vars
  uint16_t adc_avg;
  uint16_t adc_dat;

  int ch; // future use; serial input data

  // initialize board LED as progress indicator
  gpio_init(G_LED_PIN);
  gpio_set_dir(G_LED_PIN, GPIO_OUT);
  gpio_put(G_LED_PIN, 1); // initially lit to verify firmware start

  // initialize missed pulse indicator
  gpio_init(G_GP15_MISS);
  gpio_set_dir(G_GP15_MISS, GPIO_OUT);
  gpio_put(G_GP15_MISS, 0); 
  
  // begin usb serial comms
  stdio_init_all();
  #if defined(MC)
  tusb_wait_for_connection();  // optionally wait for 'mc' before capturing data
  printf("rcs-rx04-03 tx -> rx pulse capture\n");
  #endif
  const uint16_t adc_threshold = fAdcThreshold / G_adc_cf;
  //  printf("--debug-- sanity check adc_threshold: %7.5f\n", adc_threshold * G_adc_cf);

  // configure adc
  adc_init();
  adc_gpio_init(GP26_ADC0);
  adc_select_input(0);

  // configure gpio transistor bias network
  config_gpio_pullup(GP21_IREF); // pull up
  config_gpio_pullup(GP19_INP);
  config_gpio_pullup(GP12_INM);
  config_gpio_pullup(GP18_LOADP);
  config_gpio_pullup(GP13_LOADM);
  config_gpio_pulldown(GP20_INPLO); // pull down
  config_gpio_pulldown(GP11_INMLO);

  // initial adc vaules are high; this could just be settling time of the bias network; if so,
  // sleep would do, but read the adc uNsettle times instead, and use the returned average to
  // as the initial quiescent sample average.  assume no pulses are assumed during this time,
  // meaning rx must startup before tx.
  #if defined(MC)
  // printf("--debug-- capturing baseline adc_avg\n");
  #endif
  adc_avg = adc_avg_n(uNsettle); 

  // define triggers for this capture; global for access from hw timer callbacks without args
  G_uAdcTriggerPos = adc_avg + adc_threshold;
  G_uAdcTriggerNeg = adc_avg - adc_threshold; 

  gpio_put(G_LED_PIN, 0); // indicates baseline complete, waiting for trigger

  #if defined(MC)
  // printf("--debug-- wait for first pulse\n");
  #endif

  // remain in loop waiting on first pulse, no timeouts processed
  G_uFlightTimeSemaphore = time_us_64();
  wait_for_pulse(); 

  struct repeating_timer timer;
  // --20211018-- sleep_ms was fixed 2000, assuming 4000ms TX_PERIOD
  sleep_ms(TX_PERIOD/3); // delay fraction of tx01 TX_PERIODms pulse interval
  // minus '-TX_PERIOD' repeat time makes it not relative to prevous calls; fixed period of TX_PERIODms
  add_repeating_timer_ms(-TX_PERIOD, repeating_timer_callback, NULL, &timer);
  // printf("--debug-- repating hw timer is set\n");

  // sleep forever; work is all done inside hw timer callback
  while (true) {
    // busy_wait_ms(TX_PERIOD); // --20211018-- convert to sleep; time should be irrelavant
    sleep_ms(TX_PERIOD);
  }

#endif // end #ifndef PICO_DEFAULT_LED_PIN
}

/* Notes */
//  printf("--debug-- rx data:\n");
//  printf("--debug-- adc_read(): %7.5f\n", adc_read()*G_adc_cf);
//  printf("--debug-- adc_avg:    %7.5f\n", adc_avg*G_adc_cf);
//  printf("--debug-- thresh_pos: %7.5f\n", G_uAdcTriggerPos*G_adc_cf);
//  printf("--debug-- thresh_neg: %7.5f\n", G_uAdcTriggerNeg*G_adc_cf);
