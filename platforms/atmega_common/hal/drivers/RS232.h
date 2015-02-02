#include <avrlibtypes.h>

// Size of receive ring buffer. Must be at least 2.
#define USART_BUFFER_SIZE 10

/* Baudrate settings. Refer to datasheet for baud rate error.
   Note also maximun baud rate.
   br = baudrate, fosc = clock frequency in megahertzs */
#define USART_BAUDRATE(br, fosc) ((fosc*125000+br/2)/br-1)

/* Initializes USART device. Use USART_BAUDRATE macro for argument or
   consult datasheet for correct value. */
void usart_init(u32 baud_divider);

/* Transmit one character. No buffering, waits until previous character
   transmitted. */
void usart_putc(char a);

/* Transmit string. Returns after whole string transmitted. */
void usart_puts(char *data);

/* Receive one character. Blocking operation, if no new data in buffer. */
char usart_getc(void);

/* Returns number of unread character in ring buffer. */
unsigned char usart_unread_data(void);

void usart_putbyte(uint8_t val);


void usart_putu32hex(u32 num);
