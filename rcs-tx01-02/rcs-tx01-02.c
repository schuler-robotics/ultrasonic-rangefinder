// @file rcs-tx01-02.c
// @date 2021.08.27

// @info ultrasound-rangefinder transmitter
// @info control tx-01 ckt; 5V to 40Vp-p, 40Khz, drive into piezo tx
// @info switch pump (G_GP0) with 62.5KHz pwm for 400ms, drive 8 40KHz non-overlapping clock pulses (G_GP10/G_GP11)
// @info process started gated by monentary switch release (G_GP5), then periodicly repeated

// --warning-- disconnect inductor pin(40) when debuging; stuck faults will overheat switcher fet

// Copyright 2022 RC Schuler. All rights reserved.
// Use of this source code is governed by a GNU V3
// license that can be found in the LICENSE file.

// @info control tx-01 ckt; 5V to 40Vp-p, 40Khz, drive into piezo tx
// @info switch pump (G_GP0) with 62.5KHz pwm for 400ms, drive 8 40KHz non-overlapping clock pulses (G_GP10/G_GP11)
// @info process started gated by monentary switch release (G_GP5), then periodicly repeated

// @build cd build; make -j4 ;# init with mkdir build; cd build; cmake ..
// @build xfr firmware: boot w/boot_sel, cp -rp <outfile>.uf2 /media/pi/RPI_RP2

// @require pwm output G_GP0 (pin 1); pin 3 is gnd
// @require gpio outputs G_GP10, G_GP11 (pins 14, 15)
// @require switch input G_GP5 sw1 (pin7) to gnd (default pull up)
// @info "1st 8 gpio boot 50K pull up, remainder pull down; all programmable"

// @issue first pulses measures long; skipping first pulse in calculation; search 'first gpio pulse'

// @date 2021.08.27 fork from rcs-switch-pump-01
// @date 2021.08.30 added repeating timer callback; busy_wait* since sleep* not allowed in timer callbacks
// @date 2021.09.10 fixed switcher pwm bug; pwm enable in in timer_callback was not sufficient; need to re-init each
//                  callback because GP0 is re-asign to low after pulse.
// @date 2021.10.12 increase duty cycle from ( / 499.0 999) 0.499 to (/ 549.0 949) 0.578

#define TIME_CHARGE 500   // switch pump pre-charge up time in ms (--dev-- prod: 500)
#define TIME_DELAY  -2000 // timer period in ms; TIME_DELAY > TIME_CHARGE+numPulses*25us+callback_overhead
                          // negative sign: TIME_DELAY between callbacks indepencent of callback time
// #define MC                // allow mincom serial comm stdout

#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"
#include "tusb.h"  // tusb_wait_for_connection()

// globals
uint64_t G_time_us_last = 0L; // initial value
// repeating_timer_callback resources; these could be passed instead
const uint G_GP0 = 0;   // pin 1 pwm out chan A
const uint G_GP10 = 10; // pin 14 vin clock
const uint G_GP11 = 11; // pin 15 vinb clock
const uint G_GP5 = 5;   // pin 7 sw1
const float G_fClkDiv = 2;  // pwm params
// --rcs-- org pwm 50%
// const uint16_t G_uWrap = 999;
// const uint16_t G_uChanLevel = 499;
// new pwm 2021.10.12
const uint16_t G_uWrap = 949;
const uint16_t G_uChanLevel = 549;
uint G_uSliceNum;

// intentional compile error if PICO_DEFAULT_LED_PIN is not defined by sdk
const uint G_LED_PIN = PICO_DEFAULT_LED_PIN;

void tusb_wait_for_connection(void) {
  // blocks until minicom connection; MC is assumed defined before call
  while (!tud_cdc_connected()) {
    printf(".");
    sleep_ms(300);
  }
  printf("\ntusb connection established\n");
  printf("c-a x exits minicom\n");
}

uint set_pwm(uint uGpNum, float fClkDiv, uint16_t uWrap, uint16_t uChanLevel)
{
  // uGpNum valid gpio num
  // fClkDiv clock divider; e.g. 125Mhz / fClkDiv = pwm clock freq
  // uWrap pwm clock segmentation; freq_pwm = 125MHz / (uWrap+1); e.g. 125Mhz / (3+1) -> 32.5MHz
  //   with fClkDiv=2, uWrap=999 -> freq_pwm = (125MHz/2) / (999+1) -> 62Khz
  // uChanLevel sets number of 'high' cycles; uWrap/2 -> 50%
  // returns (uint)G_uSliceNum
  gpio_set_function(uGpNum, GPIO_FUNC_PWM);
  uint G_uSliceNum = pwm_gpio_to_slice_num(uGpNum);
  pwm_set_clkdiv(G_uSliceNum, fClkDiv);
  pwm_set_wrap(G_uSliceNum, uWrap);
  // @todo How is CHAN_A, CHAN_B associated with a PWM set gpio?  Order of gpio_set_function() calls?
  pwm_set_chan_level(G_uSliceNum, PWM_CHAN_A, uChanLevel);
  return G_uSliceNum;
}

bool repeating_timer_callback(struct repeating_timer *t) {
  #if defined(MC)
  uint64_t time_us_now = time_us_64(); //--dev--
  printf("tx pulse repeating_timer_callback delta time: %lld\n", time_us_now-G_time_us_last);
  G_time_us_last = time_us_now;
  #endif
  G_uSliceNum = set_pwm(G_GP0, G_fClkDiv, G_uWrap, G_uChanLevel);   // pwm init
  gpio_put(G_LED_PIN, 1); // LED high, start pre-charge
  pwm_set_enabled(G_uSliceNum, true);
  // printf("--debug-- post pwm enable\n");
  // --dev-- sleep_ms(TIME_CHARGE); --bug-- cannot sleep in timer callback?
  busy_wait_ms(TIME_CHARGE);
  // printf("--debug-- post TIME_CHARGE busy_wait\n");
  // enable gpio clocks for 8 pulses
  // 11,x,10,x = 48Khz (x== no trailing sleep for non-overlap)
  // 11,x,11,x = 41Khz
  // 12,x,12,x = ??Khz --dev-- --todo-- need scope measurment
  for ( int i=0; i<8; i++) {
    gpio_put(G_GP10, 1);
    //    sleep_us(12); // --dev-- --bug-- sleeps crash repeating_timer_callback?
    busy_wait_us(12);
    gpio_put(G_GP10, 0); // skip trailing sleep
    gpio_put(G_GP11, 1);
    //    sleep_us(12); // --dev-- --bug-- sleeps crash repeating_timer_callback?
    busy_wait_us(12);
    gpio_put(G_GP11, 0); // skip trailing sleep
  }
  // stop pwm
  pwm_set_enabled(G_uSliceNum, false);
  // printf("--debug-- post pwm disable\n");
  gpio_put(G_LED_PIN, 0); // LED low, pulse complete
  // remove pwm, re-init pin as output, low
  gpio_init(G_GP0);
  gpio_set_dir(G_GP0, GPIO_OUT);
  gpio_put(G_GP0, 0);
  return true;
}

int main() {
  // LED to indicate loaded/running firmware; pulse freq may determine parameters
  gpio_init(G_LED_PIN);
  gpio_set_dir(G_LED_PIN, GPIO_OUT);
  gpio_put(G_LED_PIN, 1);

  // begin usb serial comms
  stdio_init_all();
  #if defined(MC)
  tusb_wait_for_connection();  // optionally wait for 'mc' before capturing data
  printf("rcs-tx01-02 loop switch-pump pulses to piezo tx\n");
  #endif

  // define switch between pull up gpio and gnd
  gpio_init(G_GP5);
  gpio_set_dir(G_GP5, GPIO_IN);
  gpio_pull_up(G_GP5); // redundant for reference; G_GP5 boots pull up

  // define clock phases, initialize low
  gpio_init(G_GP10);
  gpio_set_dir(G_GP10, GPIO_OUT);
  gpio_put(G_GP10, 0);
  gpio_init(G_GP11);
  gpio_set_dir(G_GP11, GPIO_OUT);
  gpio_put(G_GP11, 0);

  // first gpio pulse train is long on first use, causing the string to measure 36Khz, instead
  // of 41Khz of subsequent pulse trains. 
  
  // initalize G_GP0 pwm clock: 125MHz/2/1000 -> 62.5Khz, 50% duty cycle
  G_uSliceNum = set_pwm(G_GP0, G_fClkDiv, G_uWrap, G_uChanLevel);
  #if defined(MC)
  // printf("--debug-- G_uSliceNum: %d\n", G_uSliceNum);
  #endif
  // pre-pwm, init pin as output, low
  gpio_init(G_GP0);
  gpio_set_dir(G_GP0, GPIO_OUT);
  gpio_put(G_GP0, 0);

  #if defined(MC)
  printf("--debug-- waiting on switch press/release\n");
  #endif
  // gate process start until switch is pressed and released
  while (!gpio_get(G_GP5)) { // wait until button high via pull up; LED is high
    sleep_ms(200);
  }
  while (gpio_get(G_GP5)) { // pull up and button not pressed
    gpio_put(G_LED_PIN, 1); // 4Hz LED flash (pulse armed)
    sleep_ms(125);
    gpio_put(G_LED_PIN, 0);
    sleep_ms(125);
  } // end while (gpio_get(
  gpio_put(G_LED_PIN, 0);
  while (!gpio_get(G_GP5)) { // wait until button high via pullup; LED is low
    sleep_ms(20);
  }

  // button pressed and relased, create repeating hw timer
  struct repeating_timer timer;
  add_repeating_timer_ms(TIME_DELAY, repeating_timer_callback, NULL, &timer);

  while (true) {
  } // end while (true) 
}
