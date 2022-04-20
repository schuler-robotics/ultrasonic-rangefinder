// @file rcs-utils-01.h
// @date 2021.09.13
// @info common pico utils header

// @require raspi pico (2020)
// @require usb connection to raspi host; internal pico LED; minicom
// @require minicom -b 115200 -o -D /dev/ttyACM0 ;# monitor serial i/o
//          c-a z -> menu, c-a x -> exit; -C or --capturefile=~/Downloads/mc.dat

// globals
// - serial comms
extern const uint8_t G_uBuf[1025];
extern uint G_uBufCt;
// - hardware
extern const uint G_LED_PIN;
extern const float  G_adc_cf; // 12 bit adc conversion factor

// serial comms
void tusb_wait_for_connection(void);
int get_serial_char();
// hardware
void flash_led_16hz();
void config_gpio_pullup(uint gp);
void config_gpio_pulldown(uint gp);
uint16_t adc_avg_n ( uint uN );
