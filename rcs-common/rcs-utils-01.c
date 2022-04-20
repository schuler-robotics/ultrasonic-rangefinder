// @file rcs-utils-01.c
// @date 2021.09.13
// @info common pico utils

// @require raspi pico (2020)
// @require usb connection to raspi host; internal pico LED; minicom
// @require minicom -b 115200 -o -D /dev/ttyACM0 ;# monitor serial i/o
//          c-a z -> menu, c-a x -> exit; -C or --capturefile=~/Downloads/mc.dat

// #define MC // mincom support; enables stdio; gates program start prior to mc connection

#include "pico/stdlib.h"
#include <stdio.h>
#include <stdlib.h>
#include "tusb.h"  // tusb_wait_for_connection()
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include <inttypes.h> // printf("%" PRIu64/d64/x64 "\n", t)

// globals
// - hardware
const uint   G_LED_PIN = PICO_DEFAULT_LED_PIN;
const float  G_adc_cf = 3.3f / (1 << 12); // 12 bit adc conversion factor
// - serial comms
uint8_t  G_uBuf[1025]; // global serial data buffer 
uint     G_uBufCt=0;   

void tusb_wait_for_connection(void) {
  // blocks until minicom connection; MC is assumed defined before call
  while (!tud_cdc_connected()) {
    printf(".");
    sleep_ms(300);
  }
  printf("\ntusb connection established\n");
  printf("c-a x exits minicom\n");
}

int get_serial_char() {
  // return a char with echo from serial; accumulate chars in global G_uBuf*
  // newline (13) completes global string and resets counter
  // return received char; globals G_uBuf* updated
  // MC is assumed defined before call
  const uint newline = 13;
  int ch = getchar_timeout_us(0);
  if (ch != PICO_ERROR_TIMEOUT) {
    printf("%c", ch);
    if (ch != newline  && G_uBufCt < 1024 ) { // newline and not char limit
      G_uBuf[G_uBufCt]=ch;
      G_uBufCt++;
    } else {
      G_uBuf[G_uBufCt]=0;
      // printf("--debug-- get_serial_char()->G_uBuf: %s\n", G_uBuf);
      G_uBufCt=0; // reset counter
    } // end if (ch != newline...
  } else {
    // 'timeout' suppressed; happens each loop pass with no input
    // printf("--debug-- get_serial_char() timeout\n", G_uBuf);
  } // end if (ch != PICO_ERROR_TIMEOUT)
  return ch;
} // end get_serial_char()

void flash_led_16hz() {
  // flash the on board LED; 31ms for 50% duty cycle 16Hz flashes;
  for (uint i=0; i<3; i++) { // 4 pulses/flashes
    gpio_put(G_LED_PIN, 1); 
    busy_wait_ms(31);
    gpio_put(G_LED_PIN, 0);
    busy_wait_ms(31);
  } // end for
} // end void flash_led_16hz() 

void config_gpio_pullup(uint gp) {
  // configure pin 'gp' as input with 50K pull up
  gpio_init(gp);
  gpio_set_dir(gp, GPIO_IN);
  gpio_pull_up(gp);
}

void config_gpio_pulldown(uint gp) {
  // configure pin 'gp' as input with 50K pull down
  gpio_init(gp);
  gpio_set_dir(gp, GPIO_IN);
  gpio_pull_down(gp);
}

uint16_t adc_avg_n ( uint uN ) {
  // take uN adc samples and return average.
  uint16_t adc_avg = adc_read(); // init average
  for (size_t i=0; i<uN; i++) {
    adc_avg = (adc_avg + adc_read())/2.0;
  }
  return adc_avg;
} // end uint16_t adc_avg_n ( uint uN )

