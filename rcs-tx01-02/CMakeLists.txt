cmake_minimum_required(VERSION 3.16)

include(pico_sdk_import.cmake)

project(rcs-tx01-02 C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()
if (TARGET tinyusb_device)

  add_executable(rcs-tx01-02
    rcs-tx01-02.c
    )

  # Pull in our pico_stdlib which pulls in commonly used features
  target_link_libraries(rcs-tx01-02 pico_stdlib hardware_pwm pico_bootsel_via_double_reset)

  # enable usb output, disable uart output
  pico_enable_stdio_usb(rcs-tx01-02 1)
  pico_enable_stdio_uart(rcs-tx01-02 0)

  # create map/bin/hex file etc.
  pico_add_extra_outputs(rcs-tx01-02)

elseif(PICO_ON_DEVICE)
  message(WARNING "not building target; TinyUSB submodule is not initialized in SDK")
endif()
