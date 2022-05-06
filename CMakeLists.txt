
  cmake_minimum_required(VERSION 3.13)

  set(CMAKE_C_STANDARD 11)
  set(CMAKE_CXX_STANDARD 17)

  include(pico_sdk_import.cmake)

  project(RC2040 C CXX ASM)
  set(CMAKE_C_STANDARD 11)
  set(CMAKE_CXX_STANDARD 17)


  add_executable(RC2040
	RC2040.c 
        ide.c 
        z80dis.c 
        acia.c
        hw_config.c
        dictionary.c
	iniparser.c
  )

  pico_sdk_init()

  add_subdirectory(FatFs_SPI build)


  add_subdirectory(libz80)
  link_directories(libz80)


  # Add the standard library and FatFS/SPI to the build
  target_link_libraries(RC2040 
    pico_multicore
    hardware_gpio 
    hardware_pwm
    pico_stdlib
    FatFs_SPI
    libz80
  )

  pico_enable_stdio_usb(RC2040 1)
  pico_enable_stdio_uart(RC2040 0)

  pico_add_extra_outputs(RC2040)


