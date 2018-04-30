/*
 * XBD_DeviceDependent.h
 *
 *  Created on: Apr 9, 2017
 *      Author: ryan
 */
// DeviceDependent header file for STM32F103RB

#ifndef INC_XBD_DEVICEDEPENDENT_H_
#define INC_XBD_DEVICEDEPENDENT_H_

#ifdef STM32F103xB
#include "stm32f1xx_hal.h"
#include "stm32f1xx_nucleo.h"
#elif defined(STM32F091xC)
#include "stm32f0xx_hal.h"
#include "stm32f0xx_nucleo.h"
#endif

/** define XBX_LITTLE_ENDIAN or XBX_BIG_ENDIAN */
#define XBX_LITTLE_ENDIAN

#if defined(STM32F091xC)
//#define XBX_BIG_ENDIAN
#endif

#define CONSTDATAAREA
#define DEVICE_SPECIFIC_SANE_TC_VALUE (16000000)

// Values taken from STM32Cube_FW_F1_V1.4.0/Projects/STM32F103RB-Nucleo/Exaples/FLASH/FLASH_EraseProgram/Inc
// Part of the STM32Cube package for STM32F103RB boards
#define ADDR_FLASH_PAGE_48    ((uint32_t)0x0800C000) //Base @ of Page 48, 1 Kbytes
#define ADDR_FLASH_PAGE_127   ((uint32_t)0x0801FC00) // Base @ of Page 127, 1 Kbytes


#define FLASH_USER_START_ADDR   ((uint32_t)0x08003000)
#define FLASH_USER_END_ADDR     ADDR_FLASH_PAGE_127 + FLASH_PAGE_SIZE   /* End @ of user Flash area */
#define FLASH_ADDR_MIN FLASH_USER_START_ADDR
#define FLASH_ADDR_MAX FLASH_USER_END_ADDR

// 1 Kbyte
#define PAGESIZE FLASH_PAGE_SIZE
#define PAGE_ALIGN_MASK 0xfffffc00

extern void printf_xbd(char *);
extern int printf_gdb(char *);
extern int print_int_gdb(unsigned int);
extern int print_int_xbd(unsigned int);

#endif /* INC_XBD_DEVICEDEPENDENT_H_ */
