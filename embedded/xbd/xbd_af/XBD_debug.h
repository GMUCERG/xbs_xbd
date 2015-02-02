#ifndef XBD_DEBUG_H_
#define XBD_DEBUG_H_


#include <stdint.h>


/** output of checksumming steps (influences checksum duration!)*/
//#define XBX_DEBUG_CHECKSUMS
/** output of ebash specific routines */
//#define XBX_DEBUG_EBASH

/** output of application */
//#define XBX_DEBUG_APP
/** output of boot loader */
//#define XBX_DEBUG_BL




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
