#include "global.h"
/** define LITTLE_ENDIAN or BIG_ENDIAN */
#define XBX_LITTLE_ENDIAN

/** prefix for const variables in static memory areas, e.g. PROGMEN for avr,
*    empty for most targets
*/
#define CONSTDATAAREA

/** define size of a page to program, keep at this value if there is no page 
*   concept */
//#define PAGESIZE 256
//May have to be shrunk, 512 is quite a large buffer.
#define PAGESIZE 512
/** mask for alignment of page boundaries, keep at this value if there is no
* page concept */

// Segment size of MSP is supposed to be 512, however program ROM starts at
// 3100.
#define PAGE_ALIGN_MASK 0xfffffe00

// Setting program start to 3200 to be safe
#define FLASH_ADDR_MIN (0x3200)
#define FLASH_ADDR_MAX (0xdfff)


#define DEVICE_SPECIFIC_SANE_TC_VALUE (32768L*32L*4L)


