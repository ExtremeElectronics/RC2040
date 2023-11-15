# Experimental Fast file transfer 

### This is experimental, but relativly stable at the moment

Currently this is only supported with the RC2040 PCB from ExtremeKits. https://extkits.co.uk/product/rc2040/ and some disk commands can be accessed if you are using the SD card module from z80kits, https://z80kits.com/shop/micro-sd-card-module/

The ideal setup would be using the serial port for the terminal, and the USB for file transfer. but you should be able to use just the USB

Currently you need to add the img diskdefs to your SD card https://github.com/ExtremeElectronics/RC2040/blob/main/DiskDefs/RC2040img_diskdefs.txt to a file called diskdefs (no extension)

By pressing the "BUT" button, the USB Serial port is freed to use with a set of tools for file transfer, directory listing and file delete
The PCB LED is set to show the RC2040 is in this mode. 

The programs are detailed here https://github.com/ExtremeElectronics/RC20XX-file-transfer-programs and should run with most installs of python 3
(Currently Linux is untested, but I can't see why it shouldn't work) 

To return back to the emulation. Run the EXIT-RC20xx.py program.

CPM doesn't know files have been dumped into a dirive externally, so you may have to move into a different drive and back again if you are in the drive you ae dumping files to, to see the changes. 

## Speed

CopyTo and CopyFrom give an idea of transfer speed, around 15Kb/s (0.16 Z/s)* should be doable for CopyTo and 52Kb/s (0.53 Z/s)* for CopyFrom

*Zorks a second, where 1 ZORK=96K
