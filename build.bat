arm-none-eabi-ld -r -b binary Floorplan.bmp -o ./floorplan.o
mbed compile -m LPC546XX -t GCC_ARM -f