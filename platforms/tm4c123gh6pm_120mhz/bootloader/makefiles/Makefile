###############################################################################
# Makefile for the project LM3S811-EVB_BL
###############################################################################

## General Flags
PROJECT = LM3S811-EVB_BL
PART=LM3S811
AXFFILE = $(PROJECT).axf
TARGET =  $(PROJECT).bin

#GCC_TYPE=eabi
GCC_TYPE=elf

CC = arm-$(GCC_TYPE)-gcc
LD = arm-$(GCC_TYPE)-ld
OBJCOPY=arm-$(GCC_TYPE)-objcopy


PATH_AF = ../../../../embedded/xbd/xbd_af
PATH_HAL = ../../hal


#
# Get the location of libgcc.a from the GCC front-end.
#
LIBGCC=${shell ${CC} ${CFLAGS} -print-libgcc-file-name}

#
# Get the location of libc.a from the GCC front-end.
#
LIBC=${shell ${CC} ${CFLAGS} -print-file-name=libc.a}

## Options common to compile, link and assembly rules

## Compile options common for all C compilation units.
#CFLAGS += -Wall -gdwarf-2 -std=gnu99      -DF_CPU=6000000UL -Os -fsigned-char
#CFLAGS += -MD -MP -MT $(*F).o -MF dep/$(@F).d 
CFLAGS=-mthumb             \
       -mcpu=cortex-m3     \
       -Os                 \
       -ffunction-sections \
       -fdata-sections     \
       -MD                 \
       -std=c99            \
       -Wall               \
       -DPART_${PART}      \
       -c
CFLAGS += -I$(PATH_HAL)/ -I$(PATH_AF) -I$(PATH_HAL)/drivers/
CFLAGS += -Dgcc

## Assembly specific flags
ASMFLAGS = $(COMMON)
ASMFLAGS += $(CFLAGS)
ASMFLAGS += -x assembler-with-cpp -Wa,-gdwarf2

## Linker flags
LDFLAGS = --gc-sections
LDFLAGS +=  -Map=$(PROJECT).map
LDFLAGS += --entry ResetISR
LDFLAGS += -T $(PATH_HAL)/XBD_BL.ld 

## Objects that must be built in order to link
OBJECTS = XBD_crc.o XBD_debug.o XBD_util.o XBD_BL.o XBD_HAL.o startup_gcc.o osram96x16x1.o XBD_commands.o
OBJECTS += sysctl.o interrupt.o uart.o gpio.o i2c.o systick.o

## Objects explicitly added by the user
LINKONLYOBJECTS = '${LIBC}' '${LIBGCC}'


## Build
all: XBD_version $(TARGET) 

## Compile
XBD_crc.o: $(PATH_AF)/XBD_crc.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

XBD_debug.o: $(PATH_AF)/XBD_debug.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<
	
XBD_commands.o: $(PATH_AF)/XBD_commands.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<
	        

XBD_util.o: $(PATH_AF)/XBD_util.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

XBD_BL.o: $(PATH_AF)/../xbd_bl/XBD_BL.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

XBD_HAL.o: $(PATH_HAL)/XBD_HAL.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

startup_gcc.o: $(PATH_HAL)/drivers/startup_gcc.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<
	
osram96x16x1.o: $(PATH_HAL)/drivers/osram96x16x1.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<	

uart.o: $(PATH_HAL)/drivers/uart.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<	

interrupt.o: $(PATH_HAL)/drivers/interrupt.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<	

sysctl.o: $(PATH_HAL)/drivers/sysctl.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<	

gpio.o: $(PATH_HAL)/drivers/gpio.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<	

systick.o: $(PATH_HAL)/drivers/systick.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<	




i2c.o: $(PATH_HAL)/drivers/i2c.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<	









	##Link
$(AXFFILE): $(OBJECTS)
	 $(LD) $(LDFLAGS) $(OBJECTS) $(LINKONLYOBJECTS) $(LIBDIRS) $(LIBS) -o $(AXFFILE)
	 
$(TARGET): $(AXFFILE)
	${OBJCOPY} -O binary $(AXFFILE) $(TARGET)


## Clean target
.PHONY: clean
clean:
	-rm -rf $(OBJECTS) $(PROJECT).elf dep/* $(PROJECT).hex $(PROJECT).eep $(PROJECT).lss $(PROJECT).map

targz:
	-tar czf $(PROJECT).tar.gz $(PROJECT).hex $(PROJECT).elf

## Other dependencies
-include $(shell mkdir dep 2>/dev/null) $(wildcard dep/*)

## Generate and autoupdate XBD_version.h
xbxdir = ../../../..
include ../../../../embedded/xbd/xbd_af/version.mk
