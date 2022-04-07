### SD card creation

Format your SD with a FAT32 file system

Copy the .bin and (unzipped) .cf files from this directory onto the CD Card.

Thats it... 

###
Rom has basic at address 0x0000 (000)
CP/M at address 0x8000 (100)
and Small Computer Monitor at 0xf000 (111)

### RC2040.INI
Config with no switches and other settings

[IDE]
idefile = "CPMIncTransient.cf"; Specify the CF file loaded as IDE drives
[ROM]
a13 = 0; // Address switches 0=0x0000 100=0x8000 111=0xF000
a14 = 0;
a15 = 1;
romfile = "R0001009.BIN"; // source for Rom Loading - see a13 a14 a15
ramonly = 0; //Ram only- load a single image of 64K - disable ROM
#romfile = "DUMP.BIN
#ramonly = 1;
[SERIAL]
#port 0=UART or 1=USB
port = 1;
[EMULATION]
# ACIA=0 SIO=1;
serialtype = 0;
[DEBUG]
trace=0; //set the internal trace logging see the Trace definitions in RC2040.c
#overclock *1000
[SPEED]
overclock = 250; overclock the PICO at 250 x 1000

