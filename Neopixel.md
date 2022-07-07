# Neopixel support

The neopixel data out pin is on GPIO 28. This is currently set to support WS2812 / WS2811 LEDS which use 24bits(RGB)
If you wish to use the 32bit LEDS you will need to recompile with WRGB set to true

The NeoPixel ports are 64-71

64 Data
65 Cmd
66 Spare(currently)
67 Led Address
68 Pallette Address
69 Red
70 Green
71 Blue

Commands

1	Set NormalMode
2	Set Pallette Mode
3	set repeat
4	set brightness
5	Set Number of pixels (Max and default 128) 
6	Spare
7	Spare
8	Spare
9	Spare
10	info (debug)
11	Spare
12	Spare
13	Spare
14	Set all from RGB values
15	All Off (RGB=0)


## Simple usage

Set the number of pixels to X by writing X to Data (port 64) then sending 5 to Cmd (port 65) to a Max of 128

Set Normal mode by writing 1 to port 1 (default)

Set the R, G and B values by writing to ports 69,70 and 71
Write the LED address to diaplay this colour into port 67

## Pallette Operation. 

Set pallette mode by writing 2 to port 1

### Set Pallette entries
Setup the pallette by writing to an RGB value to ports 69,70 and 71
Write the Pallette address to save this colour into port 68

### Using the Pallette
set the pallette address by writing to data (port 64) and write the led address into port 67

## Repeat
Repeat automatically copies the first N values (the repeat value) every n leds for the rest of the string.

Send the repeat value to data (port 64)
Send 3 to the Cmd port (65)
Write the LED values or pallette values to the first n LED's of the string. These will be copied to the end of the string.

## all on
Set the R, G and B values by writing to ports 69,70 and 71
Send 14 to the Cmd port (65)

## all off
Send 15 to the Cmd port (65)

## Brightness
To set an overall brightness (default 255 full on) 
write the brightness to Data (port 64) and then send 4 to Cmd (port 65)

## Speed
Setting the max number of pixels can increase the speed, as can not setting a brightness (brightness 255) 
This can be set either in RGB values, or via the pallette.



