
/** define XBX_LITTLE_ENDIAN or XBX_BIG_ENDIAN */
#define XBX_LITTLE_ENDIAN

/** prefix for const variables in static memory areas, e.g. PROGMEN for avr,
*    empty for most targets
*/
#define CONSTDATAAREA

/** define size of a page to program, keep at this value if there is no page 
*   concept */
#define PAGESIZE 256
/** mask for alignment of page boundaries, keep at this value if there is no
* page concept */
#define PAGE_ALIGN_MASK 0xffffff00


/** maximum flash / binary storage size */
#define FLASH_ADDR_MAX (0xffffff)

