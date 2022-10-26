# RC2040

## Z80 CP/M 80 emulation of RC2014 using the RP2040 (PI PICO) processor.
## and emulation of a handfull of hardware peripherals. 

Initial aim was to get the [EtchedPixels](https://github.com/EtchedPixels) Linux based Z80 emulation including an SD Card based IDE running on a Pi Pico as a standalone.
I have ripped out much of EtchedPixels's great work, to which I'm truly sorry, but I was only interested in the Z80 emulation, and I needed to get it to fit in an RP2040

## #Warning SPO/Beep port moved to 0x28(40)/0x29(41) to avoid clash with paging register# 

## PCB
A PCB and a full kit of parts is available here https://extkits.co.uk/product/rc2040/ for this project from Extreme Kits http://www.extkits.co.uk 

![20220514_162232](https://user-images.githubusercontent.com/102665314/168440015-87bc3225-e370-4dfc-a1a9-9be01d625213.jpg)

## Documentation

A lot more detail is available at https://www.extremeelectronics.co.uk/the-rc2040/

### Compiling

I use a RPI so...
Install Raspberry Pi SDK
Clone this RC2040 git into a subdirectory under the pico one then,

```shell
  cd pico/RC2040
  cp ../pico-sdk/external/pico_sdk_import.cmake .
  mkdir build
  cd build
  cmake ..
  make
 ```

For more details look at [Compiling.md](Compiling.md)

## SD card connection to the Pico

Details are in the Circuit diagram folder. Switches and buttons are not required. Just an SD card that can be easily attached using an old SD adapter. (diagram for this in the Circuit diagram folder)

## Serial connection details

UART 115200 N81 3ms/char delay - 3ms/line delay (1ms /3ms if overclocked at 250000)

USB  115200 N81 3ms/char delay - 3ms/line delay (1ms /3ms if overclocked at 250000)

## 8 Bit output

- port 0 mapped to 8 GPIO pins see circuit diagram
- out(0,val) will make them outputs outputting val
- in(0) will make them inputs (with pullups) returning the port state value

### ROM images

SD card images (a "get you started" is available in the [SD Card Contents] sub folder) other ROM images can be found:

- Z80 Base ROM images - https://github.com/RC2014Z80/RC2014/tree/master/ROMs/Factory
- Z80/180 Small Computer Monitor - https://smallcomputercentral.wordpress.com/small-computer-monitor/
- Z80/180 ROMWBW - https://github.com/wwarthen/RomWBW/releases
- Etched Pixels non Z80/180 cards - https://github.com/EtchedPixels/RC2014-ROM

### CP/M Disk Images (for non ROMWBW systems)

- https://github.com/RC2014Z80/RC2014/tree/master/CPM

- also note that the file format is exactly the same as the RC2014 Micro SD Card module  https://z80kits.com/shop/micro-sd-card-module/ so you can swap your SD card from the RC2040 into an RC2014 and have the same content.

### CPM manual

- http://www.cpm.z80.de/manuals/archive/cpm22htm/index.htm

### Real hardware Boards

- RC2014 - RFC2795 Ltd: https://z80kits.com
- Etched Pixels: https://hackaday.io/projects/hacker/425483
- TMS9918A card: https://www.tindie.com/stores/mfkamprath/

## Sound

Sound is output on GPIO 14/15. GPIO 15 is inverted WRT GPIO 14 so you can connect a high impedance a speaker directly across these two IO pins, a piezo speaker is ideal. A much better sound quality (and a much louder sound) can be heard using a low pass filter and an amplifier. Amplified external PC speakers work well.

### Beep

A background frequency generator can be accessed via Port 0x29 (moved from 31)
There are 126 notes defined 1-127 (from MIDI notes) sending either 0 or >128 will silence the currently playing note.

For examples, look in the basic examples folder

### SP0256-AL2

An Emulation of the SPO256-al2 chip can be accessed on port 0x28 (moved from 30)
Sending a value of 0-63 will play one of the predefined allophones that was contained in the original chip.
reading the port will give you a non-zero value if the "chip" is still playing.

See my SPO256-AL2 Git folder https://github.com/ExtremeElectronics/SP0256-AL2-Pico-Emulation-Detail for more information.
Especially the Additional folder https://github.com/ExtremeElectronics/SP0256-AL2-Pico-Emulation-Detail/tree/main/Additional

BASIC examples for sound are in the BASIC Examples folder and a script to create alophone data in BASIC is here https://extkits.co.uk/sp0256-al2/

The Allophone (decimal) numbers can be sourced from "Allophone DataSheet Addendum.txt" and the full data sheet is also in this directory which should give you an idea how to use it.

For examples, look in the basic examples folder

### Neo pixels
Neo pixel support on GPIO pin 28
default is 8 ports from a base of 64 

Set a colour using R=port 69, G=port 70, B=port 71
then set the n'th LED to that colour by writing n to port 67 

see Neopixel.md for more detail

### Fast File Serial Transfer (FFS)
Fast file serial transfer is detailed in the document https://github.com/ExtremeElectronics/RC2040/blob/main/SerialCPMtools.md
This allows direct manipulation of the CPM system on the SD card without stopping the emulation. Speeds of 500mZ/s* are achevable 

*mZ/s milli Zorks/second = time to transfer 1/1000th of Zork1 in one second.

## Progress

Now looking at documenting it and adding "useful" stuff to the build

Huge thanks goes to EtchedPixels, Grant Searle, Mitch Lalovic and Spencer Owen(Rc2014) for collating, modifying and allowing us to play with their software. and https://github.com/guidol70 for help with the img/cf file formats and diskdefs
