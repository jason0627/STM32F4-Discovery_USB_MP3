#STM32F4-Discovery_USB_MP3 (TrueStudio Compiler)
Configuration

Board : STM32F4-Discovery (STM32F407VGT)
Colpiler : Attolic TruStudio 9.0.1 for STM32
This project is based on following project. http://mikrocontroller.bplaced.net/wordpress/?page_id=735

Usage

Please set the linker script file(Debug_STM32F407VG_FLASH.ld) into following location. Project => Build setting => C/C++ Build ==> Settings => Tool Setting => Linker script

Please remove ID3v2 tag of your mp3 file.

USB memory shold be formatted with FAT32.(Default allocation size)
