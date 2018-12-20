python ./convert.py Floorplan_first_1bpp.bmp Floorplan_first.bmp
python ./convert.py Floorplan_second_1bpp.bmp Floorplan_second.bmp
arm-none-eabi-ld -r -b binary Floorplan_first.bmp -o ./floorplan_first.o
arm-none-eabi-ld -r -b binary Floorplan_second.bmp -o ./floorplan_second.o
mbed compile -m LPC546XX -t GCC_ARM
copy .\BUILD\LPC546XX\GCC_ARM\InsteonController.bin d:\