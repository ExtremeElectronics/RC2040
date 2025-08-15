
# Select your processor. or use the command line 

# cmake -DPICO_PLATFORM=rp2040 ..
# or
# cmake -DPICO_PLATFORM=rp2040 ..

#  set(PICO_PLATFORM rp2350)
#  set(PICO_PLATFORM rp2040)

string(APPEND CMAKE_EXE_LINKER_FLAGS "-Wl,--print-memory-usage")

  cmake_minimum_required(VERSION 3.13)

  include(pico_sdk_import.cmake)

  project(RC2040 C CXX ASM)
  set(CMAKE_C_STANDARD 11)
  set(CMAKE_CXX_STANDARD 17)

  pico_sdk_init()

  add_subdirectory(pico_fatfs-main)

  add_subdirectory(libz80)
  link_directories(libz80)

  add_executable(RC2040
	RC2040.c 
        ide.c 
        z80dis.c 
        acia.c
        dictionary.c
	iniparser.c
	
   #cpm tools
        cpmfs.c
        device_sdfs.c
	cpmcp.c
	cpmls.c
   #encoding	
	base64endecode.c	

   #FFS
	serialfile.c
  )

  # neopixel
  pico_generate_pio_header(RC2040  ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)

  # Add the standard library and FatFS/SPI to the build
  target_link_libraries(RC2040 
    pico_multicore
    hardware_gpio 
    hardware_pwm
    pico_stdlib
    hardware_pio 
    hardware_dma 
    hardware_irq
    hardware_flash
    hardware_spi
    pico_fatfs	
    libz80
  )

  pico_enable_stdio_usb(RC2040 1)
  pico_enable_stdio_uart(RC2040 1)

  pico_add_extra_outputs(RC2040)

# reset on baudreate change
  add_compile_definitions(PICO_STDIO_USB_ENABLE_RESET_VIA_BAUD_RATE=1)

# bootloader on double reset.
  add_compile_definitions(PICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE=0)

