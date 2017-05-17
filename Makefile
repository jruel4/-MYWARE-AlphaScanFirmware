# Simple makefile for simple example
PROGRAM=AlphaScanFirmware

EXTRA_COMPONENTS = $(CURRENT_RTOS)/extras/cpp_support
EXTRA_COMPONENTS += extras/dhcpserver
EXTRA_COMPONENTS += extras/rboot-ota extras/mbedtls
EXTRA_COMPONENTS += extras/spiffs

FLASH_SIZE = 16
# spiffs configuration
SPIFFS_BASE_ADDR = 0x100000
SPIFFS_SIZE = 0x010000

#include /media/marzipan/Expansion_1/Development/Projects/SuperHouseRTOS_2_3_17/esp-open-rtos/common.mk
include $(CURRENT_RTOS)/common.mk

$(eval $(call make_spiffs_image,files))
