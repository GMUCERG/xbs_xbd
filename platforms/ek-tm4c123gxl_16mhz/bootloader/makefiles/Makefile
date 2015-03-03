###############################################################################
# Makefile for the project TM4C123GH6PM_BL
###############################################################################

## General Flags
PROJECT = TM4C123GH6PM_BL
PART=TM4C123GH6PM
AXFFILE = $(PROJECT).axf
TARGET =  $(PROJECT).bin

#GCC_TYPE=eabi
GCC_TYPE=stellaris-eabi

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
CFLAGS+=-mthumb             \
       -mcpu=cortex-m4     \
       -mfpu=fpv4-sp-d16 -mfloat-abi=hard \
       -Os                 \
       -ffunction-sections \
       -fdata-sections     \
       -MD                 \
       -std=gnu99          \
       -Wall               \
       -DPART_${PART}      \
       -c
CFLAGS += -I$(PATH_HAL)/ -I$(PATH_AF) -I$(PATH_HAL)/drivers/
#CFLAGS += -DTARGET_IS_TM4C123_RA1
CFLAGS += -Dgcc -DBOOTLOADER
CFLAGS += -ggdb3
#CFLAGS += -flto
#CFLAGS += -ggdb3 -Og -DDEBUG -DDEBUG_BL -DDEBUG_APP

## Assembly specific flags
ASMFLAGS = $(COMMON)
ASMFLAGS += $(CFLAGS)
ASMFLAGS += -x assembler-with-cpp -Wa,-gdwarf2

## Linker flags
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Wl,-Map=$(PROJECT).map
LDFLAGS += --entry ResetISR -fuse-linker-plugin
LDFLAGS += -T ../XBD_BL.ld 

## Objects that must be built in order to link
OBJECTS += xbd_af/XBD_crc.o
OBJECTS += xbd_af/XBD_debug.o
OBJECTS += xbd_af/XBD_util.o
OBJECTS += xbd_af/XBD_commands.o
OBJECTS += xbd_af/../xbd_bl/XBD_BL.o
OBJECTS += startup_gcc.o
OBJECTS += hal/XBD_HAL.o
OBJECTS += hal/drivers/sysctl.o 
OBJECTS += hal/drivers/interrupt.o
OBJECTS += hal/drivers/uart.o
OBJECTS += hal/drivers/gpio.o
OBJECTS += hal/drivers/i2c.o
OBJECTS += hal/drivers/i2c_comm.o
OBJECTS += hal/drivers/systick.o
OBJECTS += hal/drivers/flash.o

## Objects explicitly added by the user
LINKONLYOBJECTS = '${LIBC}' '${LIBGCC}'


## Build
all: XBD_version $(TARGET) 

hal:
	-mkdir -p hal/drivers

xbd_af: 
	-mkdir xbd_af 

xbd_bl: 
	-mkdir xbd_bl

## Compile
hal/%.o: $(PATH_HAL)/%.c |hal
	$(CC) $(INCLUDES) $(CFLAGS) -c -o $@ $<

xbd_af/%.o: $(PATH_AF)/%.c |xbd_af xbd_bl
	$(CC) $(INCLUDES) $(CFLAGS) -c -o $@ $<

startup_gcc.o: ../startup_gcc.c
	$(CC) $(INCLUDES) $(CFLAGS) -c -o $@ $<


##Link
$(AXFFILE): $(OBJECTS) ../XBD_BL.ld 
	 $(LD) $(LDFLAGS) $(OBJECTS) $(LINKONLYOBJECTS) $(LIBDIRS) $(LIBS) -o $(AXFFILE)
	 
$(TARGET): $(AXFFILE)
	${OBJCOPY} -O binary $(AXFFILE) $(TARGET)


## Clean target
.PHONY: clean
clean:
	-rm -rf *.d $(OBJECTS) xbd_af xbd_bl hal $(PROJECT).elf  $(PROJECT).hex $(PROJECT).eep $(PROJECT).lss $(PROJECT).map *.axf *.bin *.tar.gz

targz:
	tar czf $(PROJECT).tar.gz $(PROJECT).axf $(PROJECT).bin

## Other dependencies
-include $(shell mkdir dep 2>/dev/null) $(wildcard dep/*)

## Generate and autoupdate XBD_version.h
xbxdir = ../../../..
include ../../../../embedded/xbd/xbd_af/version.mk
-include *.d
-include xbd_af/*.d
-include xbd_bl/*.d
