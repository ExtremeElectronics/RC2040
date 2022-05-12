# Detailed Compiling

## ADD USER PI

adduser pi

adduser pi sudo

## INSTALL PICO-SDK

git clone -b master https://github.com/raspberrypi/pico-sdk.git

cd pico-sdk

git submodule update --init

cd ..

git clone -b master https://github.com/raspberrypi/pico-examples.git

sudo apt update

sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential

sudo apt install libstdc++-arm-none-eabi-newlib

## CLONE RC2040 and COMPILE IT

su pi

export PICO_SDK_PATH=/home/pi/pico/pico-sdk

cd /home/pi/

mkdir pico

cd pico

git clone https://github.com/ExtremeElectronics/RC2040

cd pico/RC2040

cp ../pico-sdk/external/pico_sdk_import.cmake .

mkdir build

cd build

cmake ..

make

## Update SDK

cd /home/pi/pico/pico-sdk

git pull

git submodule update

Thanks to https://github.com/guidol70 for the detail
