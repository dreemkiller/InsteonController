# Introduction
Several years ago, a design for a wall-mounted light controller made the rounds on the internet(https://www.designboom.com/project/floor-plan-light-switch/). The design, by Taewon Hwang, proposed to solve the problem of a confusing bank of multiple switches by replacing them with a single plate that showed a floorplan of the residence. A user could touch the room to control the lights in that room. The design was only a mock up, and to my knowledge, the designer never followed through with a product.

When I found the NXP OM13092 LPCXpresso Development board (https://www.nxp.com/products/processors-and-microcontrollers/arm-based-processors-and-mcus/lpc-cortex-m-mcus/lpc54000-series-cortex-m4-mcus/lpcxpresso-development-board-for-lpc5460x-mcus:OM13092), this idea came back to me. It contains a 470x272 touchscreen paired with the NXP LPC54608 MCU. The LPC54608 contains an Arm Cortex-M4, on-chip flash and SRAM. It also contains an ethernet port, as well as Arduino compatible expansion headers.

This project implements Taewon Hwang's design, using the touch screen as the floor plan, and Insteon as the light control technology. I have used a Wifi Arduino expansion board for connectivity, and the LPC54608 is communicating with the Insteon hub using the network interface.
I am open to expanding it to other lighting solutions, such as ZWave, Zigbee, or even communicating to another hub technology, such as Open HAB (https://www.openhab.org/), but have not done that work, yet.

# Image Configuration
Images for the floorplans should be 480 pixels wide and 270 pixels high (480x270).
The input format to the build script requires that the format be 1 bit-per-pixel (monochrome). I've had success using MS Paint to generate these images (Macs do something weird with the header that I haven't figured out).
Due to the way the LCD wants the data, you should flip the image vertically before saving to the 1bpp format.

You should leave a certain amount of space on the left and right hand sides of the images to allow for the floor change graphics and touch area. Right now, that's 40 pixels (hard-coded. I know, there's work to do).

The 1bpp floorplan images are converted to 2 bits-per-pixel with the convert.py script. This is integrated into the 
build script, so you don't have to do anything.
The generated 2bpp floorplan images are integrated into the data section of the flashed code image using the linker.