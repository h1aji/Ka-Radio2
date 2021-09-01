PROGRAM = karadio

PROGRAM_SRC_DIR = ./src
PROGRAM_INC_DIR = ./include

# LINKER_SCRIPTS = ./ld/app.ld

EXTRA_COMPONENTS += extras/mdnsresponder
LIBS ?= gcc hal m

include $(SDK_PATH)/common.mk

CHEAT_ARG := $(shell ./web/generate.sh)
