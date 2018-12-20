#!/usr/bin/python

import struct
import sys

image_file = open(sys.argv[1], "rb")
dest_file = open(sys.argv[2], "wb")
image_data = image_file.read()
dest_image_data = []
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
