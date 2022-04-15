### SD card creation

Format your SD with a FAT32 file system

Copy the .bin and (unzipped) .cf files from this directory onto the CD Card.

Thats it... 

_______________________________________________________________________________________________________

#### NOTE CF is nolonger the default IDE format
changed to .img format with a 1K id(identification) file
if you want to use .cf add iscf=1 to the IDE section of the ini file 
_______________________________________________________________________________________________________

### INI file advanced usage.


### Using SIO based software

Requires CPM Inc Transient Apps SIO2.img,CPMIDE.id, 24886009.BIN and rc2040.ini as below

### Rom Details
Rom has basic at address 0x0000 (000) ROMsize 0x2000

Rom has basic at address 0x2000 (001) ROMsize 0x2000

CP/M / Basic via monitor at address 0x4000 (010) ROMsize 0x4000

Small Computer Monitor at 0xe000 (111) ROMsize 0x2000

More detail at [https://github.com/RC2014Z80/RC2014/tree/master/ROMs/Factory]

### RC2040.INI SIO2
Config with no switches and other emulation settings for SIO2

[IDE]
idefile = "CPM Inc Transient Apps ACIA.img"; 
idefilei = "CPMIDE.id";

[ROM]
a13 = 0; // Address switches 0=0x0000 100=0x8000 111=0xE000
a14 = 1;
a15 = 0;
// ROM file as ROM source
romfile = "24886009.BIN"; // source for Rom Loading - see a13 a14 a15

// Size of ROM
romsize=0x4000;

[CONSOLE]
// Console port 0=UART or 1=USB
port = 1;

[EMULATION]
// ACIA=0 SIO=1;
serialtype = 1; //SIO selected
//describe the ini 
inidesc="SIO using 24886009.BIN"; 

[SPEED]
vPico overclocking *1000 Mhz
overclock = 250; overclock the PICO at 250 x 1000

_______________________________________________________________________________________________________


### RC2040.INI ACIA
Config with no switches and other emulation settings for ACIA

Requires CPM Inc Transient Apps ACIA.img,CPMIDE.id, R0001009.BIN and rc2040.ini as below

### Rom Details
Rom has basic at address 0x0000 (000)

CP/M via monitor at address 0x8000 (100)

Small Computer Monitor at 0xe000 (111)

More detail at [https://github.com/RC2014Z80/RC2014/tree/master/ROMs/Factory]

### RC2040.INI
Config with no switches and other emulation settings

[IDE]
idefile = "CPM Inc Transient Apps ACIA.img"; 
idefilei = "CPMIDE.id";


[ROM]
a13 = 0; // Address switches 0=0x0000 100=0x8000 111=0xE000
a14 = 0;
a15 = 1;

// Size of ROM
romsize=0x4000;

#ROM file as ROM source
romfile = "R0001009.BIN"; // source for Rom Loading - see a13 a14 a15

[CONSOLE]
// Console port 0=UART or 1=USB
port = 1;

[EMULATION]
// ACIA=0 SIO=1; //ACIA selected
serialtype = 0;
//describe the ini 
inidesc="ACIA using R0001009.BIN"; 

[SPEED]
//Pico overclocking *1000 Mhz
overclock = 250; overclock the PICO at 250 x 1000

_______________________________________________________________________________________________________

### Other INI options. 
[ROM]
// non standard start vector (e.g not 0x0000);
jumpto = 0x2000;

// ram only (no rom, 64K load from romfile);
ramonly = 1;

[PORT]
// set the IO address of the 8 bit port
pioa=0

[IDE]
ide=0; //Turn off IDE
iscf=1; //enable cf file as idefile rather then  .img format

[EMULATION]
inidesc="Broken INI file"; //ini file description to show at boot 

[DEBUG]
trace = 0 // trace details in RC2040.c


