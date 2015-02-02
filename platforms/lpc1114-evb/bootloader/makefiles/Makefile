###############################################################################
# Makefile for the project lxp1114-EVB_BL
###############################################################################

## General Flags
PROJECT = lxp1114-EVB_BL
TARGETNAME = LXP11xx
AXFFILE = $(PROJECT).axf
TARGET =  $(PROJECT).bin

GCC_TYPE=none-eabi
#GCC_TYPE=elf

CC = arm-$(GCC_TYPE)-gcc
LD = arm-$(GCC_TYPE)-gcc
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
       -mcpu=cortex-m0     \
       -Os                 \
       -ffunction-sections \
       -fdata-sections     \
       -fmessage-length=0  \
       -fno-builtin	   \
       -std=c99            \
       -Wall               \
       -DTARGET=${TARGETNAME}	\
       -c
CFLAGS += -I$(PATH_HAL) -I$(PATH_AF) -I$(PATH_HAL)/drivers/
CFLAGS += -Dgcc -DBOOTLOADER

## Assembly specific flags
ASMFLAGS = $(COMMON)
ASMFLAGS += $(CFLAGS)
ASMFLAGS += -x assembler-with-cpp -Wa,-gdwarf2

## Linker flags
LDFLAGS = -Wl,--gc-sections --nostartfiles -mcpu=cortex-m0 -mthumb -T  $(PATH_HAL)/drivers/memconfig_bl.ld
#LDFLAGS += --entry ResetISR
#LDFLAGS += -T $(PATH_HAL)/XBD_BL.ld 

## Objects that must be built in order to link
OBJECTS = XBD_crc.o XBD_debug.o XBD_util.o XBD_BL.o XBD_HAL.o XBD_commands.o systick.o wdt.o gpio.o cpu.o LPC1xxx_startup.o LPC11xx_handlers.o iap.o

# OBJECTS += 

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

systick.o: $(PATH_HAL)/drivers/systick.c
	$(CC) $(INCLUDES) $(CFLAGS) -c $<
wdt.o: $(PATH_HAL)/drivers/wdt.c
	$(CC) $(INCLUDES) $(CFLAGS) -c $<
gpio.o: $(PATH_HAL)/drivers/gpio.c
	$(CC) $(INCLUDES) $(CFLAGS) -c $<
cpu.o:  $(PATH_HAL)/drivers/cpu.c
	$(CC) $(INCLUDES) $(CFLAGS) -c $<
iap.o:  $(PATH_HAL)/drivers/iap.c
	$(CC) $(INCLUDES) $(CFLAGS) -c $<
LPC1xxx_startup.o: $(PATH_HAL)/drivers/LPC1xxx_startup.c
	$(CC) $(INCLUDES) $(CFLAGS) -c $< 
LPC11xx_handlers.o: $(PATH_HAL)/drivers/LPC11xx_handlers.c
	$(CC) $(INCLUDES) $(CFLAGS) -c $<

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
