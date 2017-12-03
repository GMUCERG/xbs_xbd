#include <inttypes.h>
#include <stdbool.h>


/** define XBX_LITTLE_ENDIAN or XBX_BIG_ENDIAN */
#define XBX_LITTLE_ENDIAN

/** prefix for const variables in static memory areas, e.g. PROGMEN for avr,
*    empty for most targets
*/
#define CONSTDATAAREA

/** define size of a page to program, keep at this value if there is no page 
*   concept */
// This is inaccurate (for msp432 the real page size is 4096, but the XBS
// does not support sending that size payload to the XBH)
#define PAGESIZE (1024)
/** mask for alignment of page boundaries, keep at this value if there is no
* page concept */
#define PAGE_ALIGN_MASK 0xfffffc00


/** maximum flash / binary storage size */
#define FLASH_ADDR_MAX (0x0003FFFF)

/** minimum flash address, if boot loader resides below app */
#define FLASH_ADDR_MIN (0x3000)

#define DEVICE_SPECIFIC_SANE_TC_VALUE (16000000)

// Use extern end to find end of .bss instead
#define MSP432P401R_STACKTOP ((uint8_t *)0x20002000)
#define MSP432P401R_STACKBOTTOM ((uint8_t *)0x20001c00)

void printf_xbd(char *str);
