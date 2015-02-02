###############################################################################
# Makefile for the project XBD_BL_ATmega1284P_16Mhz
###############################################################################

## General Flags
PROJECT = XBD_BL_ATmega1284P_16Mhz
MCU = atmega1284p
TARGET = $(PROJECT).elf
CC = avr-gcc

## Options common to compile, link and assembly rules
COMMON = -mmcu=$(MCU)

## Compile options common for all C compilation units.
CFLAGS = $(COMMON)
CFLAGS += -Wall -gdwarf-2 -std=gnu99               -DF_CPU=16000000UL -Os -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -DBOOTLOADER 
CFLAGS += -MD -MP -MT $(*F).o -MF dep/$(@F).d 

## Assembly specific flags
ASMFLAGS = $(COMMON)
ASMFLAGS += $(CFLAGS)
ASMFLAGS += -x assembler-with-cpp -Wa,-gdwarf2

## Linker flags
LDFLAGS = $(COMMON)
LDFLAGS +=  -Wl,-Map=$(PROJECT).map
LDFLAGS += -Wl,-section-start=.text=0x1e000


## Intel Hex file production flags
HEX_FLASH_FLAGS = -R .eeprom -R .fuse -R .lock -R .signature

HEX_EEPROM_FLAGS = -j .eeprom
HEX_EEPROM_FLAGS += --set-section-flags=.eeprom="alloc,load"
HEX_EEPROM_FLAGS += --change-section-lma .eeprom=0 --no-change-warnings


## Include Directories
INCLUDES = -I"../../../../embedded/xbd/xbd_af" -I"../../hal" -I"../../../atmega_common/hal" -I"../../../atmega_common/hal/drivers" 

## Objects that must be built in order to link
OBJECTS = XBD_BL.o XBD_HAL.o XBD_debug.o XBD_crc.o XBD_util.o i2c.o RS232.o XBD_commands.o

## Objects explicitly added by the user
LINKONLYOBJECTS = 

## Build
all: XBD_version $(TARGET) $(PROJECT).hex $(PROJECT).eep $(PROJECT).lss size

## Compile
XBD_BL.o: ../../../../embedded/xbd/xbd_bl/XBD_BL.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

XBD_HAL.o: ../../hal/XBD_HAL.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

XBD_debug.o: ../../../../embedded/xbd/xbd_af/XBD_debug.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

XBD_commands.o: ../../../../embedded/xbd/xbd_af/XBD_commands.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<
        

XBD_crc.o: ../../../../embedded/xbd/xbd_af/XBD_crc.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

XBD_util.o: ../../../../embedded/xbd/xbd_af/XBD_util.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

i2c.o: ../../../atmega_common/hal/drivers/i2c.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

RS232.o: ../../../atmega_common/hal/drivers/RS232.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

##Link
$(TARGET): $(OBJECTS)
	 $(CC) $(LDFLAGS) $(OBJECTS) $(LINKONLYOBJECTS) $(LIBDIRS) $(LIBS) -o $(TARGET)

%.hex: $(TARGET)
	avr-objcopy -O ihex $(HEX_FLASH_FLAGS)  $< $@

%.eep: $(TARGET)
	-avr-objcopy $(HEX_EEPROM_FLAGS) -O ihex $< $@ || exit 0

%.lss: $(TARGET)
	avr-objdump -h -S $< > $@

size: ${TARGET}
	@echo
	@avr-size  ${TARGET}

## Clean target
.PHONY: clean
clean:
	-rm -rf $(OBJECTS) $(PROJECT).elf dep/* $(PROJECT).hex $(PROJECT).eep $(PROJECT).lss $(PROJECT).map $(PROJECT).tar.gz
	
targz:
	-tar czf $(PROJECT).tar.gz $(PROJECT).hex $(PROJECT).elf


## Other dependencies
-include $(shell mkdir dep 2>/dev/null) $(wildcard dep/*)

## Generate and autoupdate XBD_version.h
xbxdir = ../../../..
include ../../../../embedded/xbd/xbd_af/version.mk
