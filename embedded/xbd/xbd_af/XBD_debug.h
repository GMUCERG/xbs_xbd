#ifndef XBD_DEBUG_H_
#define XBD_DEBUG_H_


#include <stdint.h>


#ifdef DEBUG
#define XBD_DEBUG(x) XBD_debugOut(x)
#define XBD_DEBUG_32B(x) XBD_debugOutHex32Bit(x)
#define XBD_DEBUG_BYTE(x) XBD_debugOutHexByte(x)
#define XBD_DEBUG_CHAR(x) XBD_debugOutChar(x)
#define XBD_DEBUG_BUF(name, buf, len) XBD_debugOutBuffer(name, buf, len)
#else
#define XBD_DEBUG(x) 
#define XBD_DEBUG_32B(x)
#define XBD_DEBUG_BYTE(x)
#define XBD_DEBUG_CHAR(x)
#define XBD_DEBUG_BUF(name, buf, len)
#endif

/** output of checksumming steps (influences checksum duration!)*/
//#define XBX_DEBUG_CHECKSUMS
/** output of ebash specific routines */
//#define XBX_DEBUG_EBASH

/** output of application */
//#define XBX_DEBUG_APP
/** output of boot loader */
//#define XBX_DEBUG_BL



//Defined in XBD_HAL.c
extern void XBD_debugOut(const char *message) ;

/** outputs one byte as hex to the debug interface of the device
* @param byte The byte to output
*/
void XBD_debugOutHexByte(uint8_t byte);

/** outputs 32 bits as 8 hex chars to the debug interface of the device
 * @param toOutput The uint32 to output
 */
void XBD_debugOutHex32Bit(uint32_t toOutput);
/** outputs one character to the debug interface of the device
 * @param c The character to output
 */
void XBD_debugOutChar(uint8_t c);

/** outputs a buffer as hex blocks to the debug interface of the device 
 * @param name The name of the buffer, printed before the actual buffer output
 * @param buf The buffer
 * @param len The length of the buffer
 */
void XBD_debugOutBuffer(char *name, uint8_t *buf, uint16_t len);

#endif
