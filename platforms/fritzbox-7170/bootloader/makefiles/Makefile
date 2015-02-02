###############################################################################
# Makefile for the project XBD_BL_fritzbox-7170
###############################################################################

## General Flags
PROJECT = XBD_BL_fritzbox-7170
TARGET = $(PROJECT).elf
CC = mipsel-linux-uclibc-gcc

## Options common to compile, link and assembly rules

## Compile options common for all C compilation units.
CFLAGS = $(COMMON)
CFLAGS += -Wall -std=gnu99  -Os -DBOOTLOADER 

## Linker flags
LDFLAGS = $(COMMON)

## Include Directories
INCLUDES = -I"../../../../embedded/xbd/xbd_af" -I"../../hal" -I"../../../linux_common/hal"

## Objects that must be built in order to link
OBJECTS = XBD_BL.o XBD_HAL.o XBD_debug.o XBD_crc.o XBD_util.o XBD_commands.o

## Objects explicitly added by the user
LINKONLYOBJECTS = 


## Build
all: XBD_version $(TARGET) 


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


##Link
$(TARGET): $(OBJECTS)
	 $(CC) $(LDFLAGS) $(OBJECTS) $(LINKONLYOBJECTS) $(LIBDIRS) $(LIBS) -o $(TARGET)

## Clean target
.PHONY: clean
clean:
	-rm -rf $(OBJECTS) $(PROJECT).elf $(PROJECT).tar.gz

targz:
	-tar czf $(PROJECT).tar.gz $(PROJECT).elf
        

## Generate and autoupdate XBD_version.h
xbxdir = ../../../..
include ../../../../embedded/xbd/xbd_af/version.mk
