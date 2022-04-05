# RC2040
Z80 emulation of RC2014 using the RP2040 processor 

Initial aim was to get the EtchedPixels (https://github.com/EtchedPixels) linux based z80 emulation including an SD Card based IDE running on a Pi Pico as a standalone.
I have ripped out much of EtchedPixels's great work, to which I'm truely sorry, but I was only interested in the Z80 emulation, and I needed to get it to fit in an RP2040

# Compiling
I use a RPI so...
Install Raspberrypi SDK 
cloned this RC2040 git into a subdirectory under the pico one then,

  cd pico/RC2040
  cp ../pico-sdk/external/pico_sdk_import.cmake .
  mkdir build
  cd build
  cmake ..
  make

# Serial connection details 

UART 115200 N81 3ms/char delay - 3ms/line delay 

USB  115200 N81 3ms/char delay - 3ms/line delay 

# Progress

Now looking at documenting it and adding "usefull" stuff to the build

Huge thanks goes to EtchedPixels, Grant Searle , Mitch Lalovic and Spencer Owen(Rc2014) for collating, modifying and allowing us to play with their software. 

SD card images (a get you started , is available in the SD Card Contents sub folder) other rom images can be found :- 


### ROM images:
- Z80 Base ROM images - https://github.com/RC2014Z80/RC2014/tree/master/ROMs/Factory
- Z80/180 Small Computer Monitor - https://smallcomputercentral.wordpress.com/small-computer-monitor/
- Z80/180 ROMWBW - https://github.com/wwarthen/RomWBW/releases
- Etched Pixels non Z80/180 cards - https://github.com/EtchedPixels/RC2014-ROM

### CP/M Disk Images (for non ROMWBW systems)

https://github.com/RC2014Z80/RC2014/tree/master/CPM

### Real hardware Boards 

- RC2014 - RFC2795 Ltd: https://z80kits.com
- Small Computer Central: https://www.tindie.com/stores/tindiescx/
- Etched Pixels: https://hackaday.io/projects/hacker/425483
- TMS9918A card: https://www.tindie.com/stores/mfkamprath/
