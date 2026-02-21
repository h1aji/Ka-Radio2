PROGRAM = karadio

PROGRAM_SRC_DIR = ./src
PROGRAM_INC_DIR = ./include

# LINKER_SCRIPTS = ./ld/app.ld

EXTRA_COMPONENTS = extras/mbedtls extras/mdnsresponder 

# extras/ssd1306 extras/i2c extras/fonts


# For the mDNS responder included with lwip:
EXTRA_CFLAGS += -DLWIP_MDNS_RESPONDER=1 -DLWIP_NUM_NETIF_CLIENT_DATA=1 -DLWIP_NETIF_EXT_STATUS_CALLBACK=1

LIBS ?= gcc hal m

include $(SDK_PATH)/common.mk

## CHEAT_ARG := $(shell ./web/generate.sh)

erase:
	echo "Erasing firmware ..."
	~/esp/esp-open-sdk/esptool/esptool.py \
                --baud 921600 --port /dev/ttyUSB0 write_flash \
                0x03fe000 docs/blank.bin \
                0x0fe000 docs/blank.bin \
                0x7E000 docs/blank.bin \
                0x3fc000 docs/esp_init_data_default.bin \
                0xFC000 docs/esp_init_data_default.bin \
                --flash_mode dio --flash_freq 40m --flash_size 4MB
