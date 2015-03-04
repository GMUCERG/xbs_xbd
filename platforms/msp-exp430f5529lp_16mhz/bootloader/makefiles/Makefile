###############################################################################
# Makefile for the project XBD_BL_MSP430F5529_4MHz
###############################################################################

## General Flags
PROJECT = XBD_BL_MSP430F5529_4MHz
MCU = msp430f5529
TARGET = bl.elf
CC = msp430-elf-gcc

MSP430GCC_INC := $(shell msp430-elf-gcc -xc -E -v - </dev/null 2>&1 | awk '/^\#include </ {getline; print $$0}')
SUPPORT_DIR := $(realpath ${MSP430GCC_INC}/../../../../../include)

## Options common to compile, link and assembly rules
COMMON = -mmcu=$(MCU) -I${SUPPORT_DIR}

## Compile options common for all C compilation units.
CFLAGS += $(COMMON)
CFLAGS += -MD -MP -MT $(*F).o -MF dep/$(@F).d 
CFLAGS += -Wall -ggdb3 -std=gnu99 -Os -DBOOTLOADER -ffunction-sections -fdata-sections
#CFLAGS += -O0 #-DXBX_DEBUG_BL 

## Assembly specific flags
AFLAGS = $(COMMON)
AFLAGS += $(CFLAGS)
AFLAGS += -x assembler-with-cpp -Wa,-ggdb3

## Linker flags
LDFLAGS = $(COMMON)
LDFLAGS +=  -Wl,-Map=bl.map
#LDFLAGS +=  -L${SUPPORT_DIR} -Wl,--gc-sections,--section-start=.rodata=0xd800
LDFLAGS +=  -L${SUPPORT_DIR} -Wl,--gc-sections,--section-start=.rodata=0xe000



## Intel Hex file production flags
HEX_FLASH_FLAGS = -R .eeprom -R .fuse -R .lock -R .signature

HEX_EEPROM_FLAGS = -j .eeprom
HEX_EEPROM_FLAGS += --set-section-flags=.eeprom="alloc,load"
HEX_EEPROM_FLAGS += --change-section-lma .eeprom=0 --no-change-warnings


## Include Directories
INCLUDES += -I../../../../embedded/xbd/xbd_af -I../../hal  -I../../hal/drivers
#-I"../../../atmega_common/hal" -I"../../../atmega_common/hal/drivers" 

## Objects that must be built in order to link
OBJECTS = XBD_BL.o XBD_HAL.o XBD_debug.o XBD_commands.o XBD_crc.o XBD_util.o i2c.o clock_init.o RS232.o #LowLevelFunc.o JTAGfunc.o


## Objects explicitly added by the user
LINKONLYOBJECTS = 


## Build
all: $(TARGET) bl.hex bl.eep bl.lss size


## Compile
XBD_BL.o: ../../../../embedded/xbd/xbd_bl/XBD_BL.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

XBD_HAL.o: ../../hal/XBD_HAL.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

XBD_debug.o: ../../../../embedded/xbd/xbd_af/XBD_debug.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

XBD_crc.o: ../../../../embedded/xbd/xbd_af/XBD_crc.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

XBD_commands.o: ../../../../embedded/xbd/xbd_af/XBD_commands.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

XBD_util.o: ../../../../embedded/xbd/xbd_af/XBD_util.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

clock_init.o: ../../hal/drivers/clock_init.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

i2c.o: ../../hal/drivers/i2c.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

#LowLevelFunc.o: ../../hal/drivers/LowLevelFunc.c
#	$(CC) $(INCLUDES) $(CFLAGS) -c  $<
#
#JTAGfunc.o: ../../hal/drivers/JTAGfunc.c
#	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

RS232.o: ../../hal/drivers/RS232.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

##Link
$(TARGET): $(OBJECTS)
	 $(CC) $(LDFLAGS) $(OBJECTS) $(LINKONLYOBJECTS) $(LIBDIRS) $(LIBS) -o $(TARGET)

%.hex: $(TARGET)
	msp430-elf-objcopy -O ihex $(HEX_FLASH_FLAGS)  $< $@

%.eep: $(TARGET)
	msp430-elf-objcopy $(HEX_EEPROM_FLAGS) -O ihex $< $@ || exit 0

%.lss: $(TARGET)
	msp430-elf-objdump -h -S $< > $@

size: ${TARGET}
	@echo
	@msp430-elf-size  ${TARGET}

## Clean target
.PHONY: clean
clean:
	-rm -rf $(OBJECTS) bl.elf dep/* bl.hex bl.eep bl.lss bl.map


## Other dependencies
-include $(shell mkdir dep 2>/dev/null) $(wildcard dep/*)

## Generate and autoupdate XBD_version.h
xbxdir = ../../../..
include ../../../../embedded/xbd/xbd_af/version.mk
