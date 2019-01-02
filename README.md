# Introduction
Several years ago, a design for a wall-mounted light controller made the rounds on the internet(https://www.designboom.com/project/floor-plan-light-switch/). The design, by Taewon Hwang, proposed to solve the problem of a confusing bank of multiple switches by replacing them with a single plate that showed a floorplan of the residence. A user could touch the room to control the lights in that room. The design was only a mock up, and to my knowledge, the designer never followed through with a product.

When I found the NXP OM13092 LPCXpresso Development board (https://www.nxp.com/products/processors-and-microcontrollers/arm-based-processors-and-mcus/lpc-cortex-m-mcus/lpc54000-series-cortex-m4-mcus/lpcxpresso-development-board-for-lpc5460x-mcus:OM13092), this idea came back to me. It contains a 470x272 touchscreen paired with the NXP LPC54608 MCU. The LPC54608 contains an Arm Cortex-M4, on-chip flash and SRAM. It also contains an ethernet port, as well as Arduino compatible expansion headers.

This project implements Taewon Hwang's design, using the touch screen as the floor plan, and Insteon as the light control technology. I have used a Wifi Arduino expansion board for connectivity, and the LPC54608 is communicating with the Insteon hub using the network interface.
I am open to expanding it to other lighting solutions, such as ZWave, Zigbee, or even communicating to another hub technology, such as Open HAB (https://www.openhab.org/), but have not done that work, yet.

# Hardware
## NXP OM13092
## WizFi310
# Software
## Mbed-OS
## My Stuff


# Set Up
## Configure Wifi
Edit the "wifi-ssid" and "wifi-password" fields of the `mbed_app.json` file to match the 
Wifi network being used. Every change to this file will require a rebuild of the project.

## Collect Insteon Information
You need to collect the device IDs of all of the insteon devices and groups that you want to control.
## Draw Up Floorplan

### Image Configuration
Images for the floorplans should be 480 pixels wide and 270 pixels high (480x270).
The input format to the build script requires that the format be 1 bit-per-pixel (monochrome). I've had success using MS Paint to generate these images (Macs do something weird with the header that I haven't figured out).
Due to the way the LCD wants the data, you should flip the image vertically before saving to the 1bpp format.

You should leave a certain amount of space on the left and right hand sides of the images to allow for the floor change graphics and touch area. Right now, that's 40 pixels (hard-coded. I know, there's work to do).

The 1bpp floorplan images are converted to 2 bits-per-pixel with the convert.py script. This is integrated into the 
build script, so you don't have to do anything.
The generated 2bpp floorplan images are integrated into the data section of the flashed code image using the linker.

## Region Set Up

# Building
Since this project uses Mbed OS, it is easiest to build it using the Mbed tools. Follow the instructions for installing the Mbed CLI tools for your platform here (https://os.mbed.com/docs/v5.11/tools/installation-and-setup.html).

Once the Mbed CLI tools are installed, you can build the project using the build script:
./build.bat
(Note, this is currently a windows script. Tune in later for improvements)

# TODO
- The board contains 16M SDRAM, start address 0xa0000000. Figure out how we might be able to use that for heap memory. "The MCUXpresso SDK includes drivers that have optimized external memory settings for use of this memory.". I think this is in the fsl_emc.h, fsl_emc.c files. Looking at the files, I think we need to call the initialization functions, but after that, we should be able to access the addresses directly.
- The board contains 128Mb SPI Flash. Figure out how we can use this. The NXP SDK claims to have drivers for this. I think these are in fsl_eeprom.h and fsl_eeprom.c.
- There is a current measurement chip on the board (MAX9634T). Documentation claims that the NXP IDE contains utilities for this measurement. We should do that.