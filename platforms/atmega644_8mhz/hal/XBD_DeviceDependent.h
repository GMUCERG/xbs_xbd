#include <avr/pgmspace.h>

#define XBX_LITTLE_ENDIAN

#define FLASH_ADDR_MAX (0x7000l * 2)
#define PAGE_ALIGN_MASK 0xffffff00

#define CONSTDATAAREA PROGMEM

#define PAGESIZE SPM_PAGESIZE
