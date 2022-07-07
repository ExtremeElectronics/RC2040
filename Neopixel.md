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

## Simple usage

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
Write the LED values or pallette values to the first n LED's of the string.

## all on
Set the R, G and B values by writing to ports 69,70 and 71
Send 14 to the Cmd port (65)

## all off
Send 15 to the Cmd port (65)





