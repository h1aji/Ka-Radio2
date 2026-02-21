#!/bin/bash

$(HOME)/esp/esp-open-sdk/esptool/esptool.py --baud 921600 --port /dev/ttyUSB0 write_flash \
        0x03fe000 ./blank.bin \
        0x0fe000 ./blank.bin \
        0x7E000 ./blank.bin \
        0x3fc000 ./esp_init_data_default.bin \
        0xFC000 ./esp_init_data_default.bin \
        --flash_mode dio --flash_freq 40m --flash_size 4MB
