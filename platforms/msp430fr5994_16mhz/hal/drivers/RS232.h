//#include "msp430libtypes.h"
#include<inttypes.h>

// Size of receive ring buffer. Must be at least 2.
#define USART_BUFFER_SIZE 10

/**
 * Wrapper for debug to usart with file and line number
 * @param ... String
 */
#ifdef DEBUG
#define XBD_debugLine(str_out) {usart_puts(__FILE__); usart_puts(": ");usart_putint(__LINE__); usart_puts(": "); usart_puts(str_out);}
#else
#define XBD_debugLine(str_out)
#endif

/* Baudrate settings. Refer to datasheet for baud rate error.
   Note also maximun baud rate.
   br = baudrate, fosc = clock frequency in megahertzs */
//Commenting out and hardcoding to 115000 to simplify
//TODO Genercize this to take into account differing F_CPU values
//
//#define USART_BAUDRATE(br, fosc) ((fosc*125000+br/2)/br-1)

///* Initializes USART device. Use USART_BAUDRATE macro for argument or
//   consult datasheet for correct value. */
void usart_init();

/* Transmit one character. No buffering, waits until previous character
   transmitted. */
void usart_putc(char a);

/* Transmit string. Returns after whole string transmitted. */
void usart_puts(const char *data);

/* Receive one character. Blocking operation, if no new data in buffer. */
char usart_getc(void);

/* Returns number of unread character in ring buffer. */
unsigned char usart_unread_data(void);

void usart_putbyte(uint8_t val);

void usart_putint (int usart_num);

void usart_putu32_hex(uint32_t num);


