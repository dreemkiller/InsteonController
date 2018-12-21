#!/usr/bin/python

import struct
import sys
import array

image_file = open(sys.argv[1], "rb")
dest_file = open(sys.argv[2], "wb")
image_data = image_file.read()
image_file.close()
for i in range(len(image_data)):
    this_byte = ord(image_data[i]);
    new_bytes =  (this_byte & 0x01) << 0;
    new_bytes += (this_byte & 0x02) << 1;
    new_bytes += (this_byte & 0x04) << 2;
    new_bytes += (this_byte & 0x08) << 3;

    new_bytes += (this_byte & 0x10) << 4;
    new_bytes += (this_byte & 0x20) << 5;
    new_bytes += (this_byte & 0x40) << 6;
    new_bytes += (this_byte & 0x80) << 7;

    dest_file.write(struct.pack('>H', new_bytes))

dest_file.close()

image_file = open(sys.argv[2], "rb")
image_data = image_file.read()
image_data = array.array('B', image_data)
image_file.close()
dest_file = open(sys.argv[2], "wb")
row_size_bytes = 480 * 2 / 8
if sys.argv[3] == '0':
    # add graphic on the left side
    
    for row in range(270):
        for i in range(10):
            #print("image_data before:" + str(image_data[row_size_bytes * row + i]))
            #data = struct.pack('B', 0x55)
            image_data[row_size_bytes * row + i] = 0x00
            #print("image_data after:" + str(image_data[row_size_bytes * row + i]))
        
elif sys.argv[3] == '1':
    # add graphic on the right side
    for row in range(270):
        for i in range(110, 120):
            #data = struct.pack('B', 0x55)
            image_data[row_size_bytes * row + i] = 0x00
dest_file.write(image_data)
dest_file.close()
