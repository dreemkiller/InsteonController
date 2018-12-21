# Image Configuration
Images for the floorplans should be 480 pixels wide and 270 pixels high (480x270).
The input format to the build script requires that the format be 1 bit-per-pixel (monochrome). I've had success using MS Paint to generate these images (Macs do something weird with the header that I haven't figured out).
Due to the way the LCD wants the data, you should flip the image vertically before saving to the 1bpp format.

You should leave a certain amount of space on the left and right hand sides of the images to allow for the floor change graphics and touch area. Right now, that's 40 pixels (hard-coded. I know, there's work to do).

The 1bpp floorplan images are converted to 2 bits-per-pixel with the convert.py script. This is integrated into the 
build script, so you don't have to do anything.
The generated 2bpp floorplan images are integrated into the data section of the flashed code image using the linker.