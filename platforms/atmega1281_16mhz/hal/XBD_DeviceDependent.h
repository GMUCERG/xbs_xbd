#include <avr/pgmspace.h>

#define XBX_LITTLE_ENDIAN

#define FLASH_ADDR_MAX (0xF000l * 2)
#define PAGE_ALIGN_MASK 0xffffff00

#define CONSTDATAAREA PROGMEM

#define FLASHGT64K 1

#ifdef BOOTLOADER
	#define UPPER64K 1
#endif


#define PAGESIZE SPM_PAGESIZE

#define DEVICE_SPECIFIC_SANE_TC_VALUE 16000000
