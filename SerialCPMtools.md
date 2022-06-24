# Experimental Fast file transfer 

### This is _very_ experimental at the moment

Currently this is only supported with the RC2040 PCB from ExtremeKits. https://extkits.co.uk/product/rc2040/

The ideal setup would be using the serial port for the terminal, and the USB for file transfer. but you should be able to use just the USB

By pressing the "BUT" button, the USB Serial port is freed to use with a set of tools for file transfer, directory listing and file delete
The PCB LED is set to show the RC2040 is in this mode. 

The programs are detailed here https://github.com/ExtremeElectronics/RC20XX-file-transfer-programs and should run with most installs of python 3
(Currently Linux is untested, but I can't see why it shouldn't work) 

To return back to the emulation. Run the EXIT-RC20xx.py program.

CPM doesn't know files have been dumped into a dirive externally, so you may have to move into a different drive and back again if you are in the drive you ae dumping files to, to see the changes. 
