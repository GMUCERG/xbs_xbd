#include <inttypes.h>
#include <stdbool.h>
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "inc/hw_ints.h"
#include "inc/hw_flash.h"
#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"




/** define XBX_LITTLE_ENDIAN or XBX_BIG_ENDIAN */
#define XBX_LITTLE_ENDIAN

/** prefix for const variables in static memory areas, e.g. PROGMEN for avr,
*    empty for most targets
*/
#define CONSTDATAAREA

/** define size of a page to program, keep at this value if there is no page 
*   concept */
//#define PAGESIZE (MAP_SysCtlFlashSectorSizeGet())
#define PAGESIZE (1024)
/** mask for alignment of page boundaries, keep at this value if there is no
* page concept */
#define PAGE_ALIGN_MASK 0xfffffc00


/** maximum flash / binary storage size */
#define FLASH_ADDR_MAX (0x0003FFFF)

/** minimum flash address, if boot loader resides below app */
#define FLASH_ADDR_MIN (0x2800)

#define DEVICE_SPECIFIC_SANE_TC_VALUE (16000000)

// Use extern end to find end of .bss instead
//#define TM4C123GH6PM_STACKTOP ((uint8_t *)0x20002000)
//#define TM4C123GH6PM_STACKBOTTOM ((uint8_t *)0x20001c00)
