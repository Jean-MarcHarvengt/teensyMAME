# teensyMAME

M.A.M.E = Multi Arcade Machine Emulator

# News

<p align="center">
<img src="/images/teensy4SD.png" width="200" />  
</p>


# Featuring
Emulation of various Arcade systems (1978-1985) from Space Invaders to Moon Patrol.<br>
The emulator is splitted in few projects running specific systems due to memory usage.<br>
(see complete list of roms emulated in games.txt)<br> 
It is based on the early MAME0.36 source code <br>
Please compile for smallest code on the Teensy4.0 else you will run out of memory<br><br>

# Minimal requirements:
- Teensy4.0
- ILI9341 or ST7789 SPI display
- SD card (Teensy uses built-in uSD)
- Analog joypad (Arduino or PSP like)
- 3 buttons (FIRE, USER1 and USER2)

# Optional requirements:
- I2C custom keyboard (not required for MAME)
- Sound (MQS only, to be added)

# Wiring
- see pinout.txt file in the project
- Main PCB KICAD also available

# I2C keyboard (not required for MAME)
- see i2ckeyboard sub-directory
- the I2C keyboard is using a separate atmega328p MCU handling the keys matrix
- with 10x4 or 10x5 keys

# Installation
- Format the SD card as FAT32
- copy the roms into a folder called "mame"
- I don't provide information about where to find the roms but just be aware you will need a romset compatible with mame 0.36. 
- MAME roms should be uncompressed (sub-dir with rom name with roms inside)
- insert the card into the SD slot

# Compilation/flashing (Teensy)
- open the ino file with Arduino SDK
- select DISPLAY MODULE in tft_t_dma_config.h (ST7789 or ILI9341)!!!!
- double check iopins.h for pins configuration!!!!
- compile and install from there.

# Status and known issues
- No sound
- some games as Bagman are not operational

# Running
- For the launched MAME version you should see the roms of the SD card being listed
- you can select the rom with up/down 
- you can start the game by pressing the FIRE key
- USER2=CREDITS and USER1=START
- You can then play the game with the analog joystick and the FIRE key  
