#===============================================================
# Makefile for building MSP Code Examples in command line
# environment using the GCC Open Source Compiler for MSP432
# Modified by Calvin Rose
# Further modified by GMU CERG
#===============================================================

# You should only need to change stuff here for the most part.

SOURCES := $(patsubst src/%.c, %.c, $(wildcard src/*.c))
TARGET := xbd.out
OBJ_DIR := output

######################################

PATH_AF = ../../../../embedded/xbd/xbd_af
PATH_BL =../../../../embedded/xbd/xbd_bl

# GCC Configuration
DEVICE ?= MSP432P401R
GCC_MSP_INC_DIR	?= ../../hal/inc
GCC_CMSIS_INC_DIR ?= $(GCC_MSP_INC_DIR)/CMSIS
LDDIR := $(GCC_MSP_INC_DIR)/
RM := rm -rf
MKDIR := mkdir -p -- $@
GCC_INC_DIR ?= /usr/lib/arm-none-eabi/include
CC := arm-none-eabi-gcc
GDB := arm-none-eabi-gdb
INCLUDES := -I $(GCC_CMSIS_INC_DIR) -I $(GCC_MSP_INC_DIR) -I $(GCC_INC_DIR) -I ../../hal/drivers -I ../../hal/
INCLUDES += -I$(PATH_BL)
INCLUDES += -I$(PATH_AF)
COMMONFLAGS := -mcpu=cortex-m4 -march=armv7e-m -mfloat-abi=hard -mfpu=fpv4-sp-d16 -mthumb -D__$(DEVICE)__ -DTARGET_IS_MSP432P4XX -Dgcc -g -gstrict-dwarf -Wall
CFLAGS := $(COMMONFLAGS) -ffunction-sections -fdata-sections -MD -std=c99
LDFLAGS := $(COMMONFLAGS) -T$(LDDIR)XBD.ld -l'c' -l'gcc' -l'nosys'


SOURCES   += startup_msp432p401r_gcc.c
SOURCES   += system_msp432p401r.c
SOURCES   += XBD_BL.c
SOURCES   += XBD_crc.c
#SOURCES   += XBD_debug.c
SOURCES   += XBD_util.c
SOURCES   += XBD_commands.c
SOURCES   += XBD_HAL.c


# Use vpath to find source files
VPATH := ../../hal:../../hal/drivers
VPATH     += $(PATH_AF)
VPATH     += $(PATH_BL)


# Get the destination object files for all source files.
OBJECTS := $(addsuffix .o,$(addprefix $(OBJ_DIR)/, $(basename $(SOURCES))))

######################################

all: XBD_version $(OBJ_DIR)/$(TARGET)

$(OBJECTS): | $(OBJ_DIR)

$(OBJ_DIR):
	@$(MKDIR) $(OBJ_DIR)

$(OBJ_DIR)/%.o: %.c
	@echo ============================================
	@echo Generating $@
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OBJ_DIR)/$(TARGET): $(OBJECTS)
	@echo ============================================
	@echo Linking objects and generating output binary
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@ $(INCLUDES)

debug: all
	$(GDB) $(OBJ_DIR)/$(TARGET)

clean:
	@$(RM) $(OBJ_DIR)

## Generate and autoupdate XBD_version.h
xbxdir = ../../../..
include ../../../../embedded/xbd/xbd_af/version.mk
-include *.d
-include xbd_af/*.d
-include xbd_bl/*.d

