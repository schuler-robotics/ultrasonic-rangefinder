cmake_minimum_required(VERSION 3.16)

include(pico_sdk_import.cmake)

project(rcs-rx04-03 C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()
if (TARGET tinyusb_device)

  add_executable(
    rcs-rx04-03
    rcs-rx04-03.c
    ../rcs-common/rcs-utils-01.c
    )

  # Pull in our pico_stdlib which pulls in commonly used features
  target_link_libraries(rcs-rx04-03 pico_stdlib hardware_adc pico_bootsel_via_double_reset)

  # enable usb output, disable uart output
  pico_enable_stdio_usb(rcs-rx04-03 1)
  pico_enable_stdio_uart(rcs-rx04-03 0)

  # create map/bin/hex file etc.
  pico_add_extra_outputs(rcs-rx04-03)

elseif(PICO_ON_DEVICE)
  message(WARNING "not building target; TinyUSB submodule is not initialized in SDK")
endif()


