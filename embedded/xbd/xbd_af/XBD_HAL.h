#ifndef __XBD_HAL_H__
#define __XBD_HAL_H__

#include <stdint.h>

#include <XBD_DeviceDependent.h>



#ifdef XBX_LITTLE_ENDIAN
#ifdef __GNUC__
    #define NTOHL(x) __builtin_bswap32(x)
    #define NTOHS(x) __builtin_bswap16(x)
    #define HTONL(x) __builtin_bswap32(x)
    #define HTONS(x) __builtin_bswap16(x)
#else
	#define NTOHL(x) ((x & 0xFF000000)>>24)+((x & 0x00FF0000)>>8)+((x & 0x0000FF00)<<8)+((x & 0x000000FF)<<24)
	#define HTONL(x) ((x & 0xFF000000)>>24)+((x & 0x00FF0000)>>8)+((x & 0x0000FF00)<<8)+((x & 0x000000FF)<<24)
	#define NTOHS(x) (((x & 0xFF00)>>8)+((x & 0x00FF)<<8))
	#define HTONS(x) (((x & 0xFF00)>>8)+((x & 0x00FF)<<8))
#endif
#else
	#define NTOHL(x) (x)
	#define HTONL(x) (x) 
	#define NTOHS(x) (x)
	#define HTONS(x) (x) 
#endif



/** Helper method to fetch a string from const data storage
* @param dest The destination for the string
* @param source The source of the string
* @note The caller has to make sure there's enough space in dest
*/
void XBD_loadStringFromConstDataArea( char * dest , const char * source);

/** Switches to XBD to bootloader mode. If already in bootloader mode this function must not do anything */
void XBD_switchToBootLoader();

/** Switches to XBD to application mode. If already in application mode this function must not do anything */
void XBD_switchToApplication();

/** Initialises the device */
void XBD_init();

/** Pumps the communication to the XBH */
void XBD_serveCommunication();

/** Sends the signal "start-of-execution" to the XBH */
void XBD_sendExecutionStartSignal();

/** Sends the signal "end-of-execution" to the XBH */
void XBD_sendExecutionCompleteSignal();

/** 
 * Busy Loops for approximately the time specified and
 * returns the exact numer of cycles elapsed.
 * Also toggles the timing pin to the XBH.
 **/
uint32_t XBD_busyLoopWithTiming(uint32_t approxCycles);

/** 
 * Busy Loops for approximately the time specified, no
 * returns value.
 **/
void XBD_delayCycles(uint32_t approxCycles);

/** Outputs a single char to the debug interface of the device 
* @param message The char to output
*/
void XBD_debugOut(const char * message );

/** Reads a block of application code to the buffer 
* @param pageStartAddress The start address of the page in byte
* @param buf The buffer to copy to
*/
void XBD_readPage( uint32_t pageStartAddress, uint8_t * buf );

/** Writes a block of application code from the buffer to the executable area of the device 
 * @param pageStartAddress The start address of the page in byte
 * @param buf The buffer to copy from
*/
void XBD_programPage( uint32_t pageStartAddress, uint8_t * buf );

/** Paints the stack area from the current stack pointer position to the end of the .bss
 * segment with a 'canary bird' pattern. This is used to find out how much stack was used
 * between painting the stack and later counting the used bytes.
 * Modified version of the code posted by MichaelMcTernan on the avrfreaks forum.
 * @global uint8_t *p_stack;
 * @return none
*/
void XBD_paintStack(void);

/** Counts the unused stack area from the last stack byte to the current stack 
 * pointer position  by looking for the 'canary bird' pattern. 
 * @global uint8_t *p_stack;
 * @return uint32_t: the minimum amount of free stack since the last call to XBD_paintStack().
*/
uint32_t XBD_countStack(void);

/** Stats sort of a watchdog for XBDs with no external reset capability
 * e.g. embedded devices that run linux 
 * the most common implementation is working with alarm() and signal().
 * XBDs with external reset capability leave the implementation empty.
 * @param seconds mimmm of seconds to wait before exiting the app
*/

void XBD_startWatchDog(uint32_t seconds);

/** Stops the XBD App watchdog 
*/
void XBD_stopWatchDog();

#endif
