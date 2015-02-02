#ifndef __XBD_FRW_H__
#define __XBD_FRW_H__

#include <stdint.h>



/** handles an incoming message
* @param len The length of the received message
* @param data The actual message
*/
void FRW_msgRecHand ( uint8_t len, uint8_t * data );

/** Queries for a message to send. As the concept for data transmission is taken from i2c, the application may only transmit a message when asked to.
* @param maxlen The maximum length of the message buffer
* @param data The message buffer
* @returns The size of the message constructed in data
*/
uint8_t FRW_msgTraHand ( uint8_t maxlen, uint8_t * data );
#endif
