PROGRAM = karadio

PROGRAM_SRC_DIR = ./src
PROGRAM_INC_DIR = ./include

# LINKER_SCRIPTS = ./ld/app.ld

EXTRA_COMPONENTS = extras/mbedtls extras/mdnsresponder

# For the mDNS responder included with lwip:
EXTRA_CFLAGS += -DLWIP_MDNS_RESPONDER=1 -DLWIP_NUM_NETIF_CLIENT_DATA=1 -DLWIP_NETIF_EXT_STATUS_CALLBACK=1

LIBS ?= gcc hal m

include $(SDK_PATH)/common.mk

## CHEAT_ARG := $(shell ./web/generate.sh)
