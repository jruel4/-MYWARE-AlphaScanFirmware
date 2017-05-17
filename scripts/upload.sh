#!/bin/bash -x

tftp 192.168.2.11 69 << TST
binary << TST
put firmware/AlphaScanFirmware.bin firmware.bin
