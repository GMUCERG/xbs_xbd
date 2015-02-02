#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <avr/interrupt.h>
#include <stdint.h>

#include "RS232.h"




void usart_init_old(unsigned char baud_divider) 
{
  // Baud rate selection
  UBRR0H = 0x00;       
  UBRR0L = baud_divider;

  // USART setup
  UCSR0A = 0x02;        // 0000 0010
                       // U2X enabled
  UCSR0C = 0x06;        // 0000 0110
                       //  UCSRC to, Asyncronous 8N1
  UCSR0B = 0x98;        // 1001 1000
                       // Receiver enabled, Transmitter enabled
                       // RX Complete interrupt enabled
}

void usart_init(u32 baudrate)
{
	usart_init_old(USART_BAUDRATE(baudrate,F_CPU/1000000)); 
}

void usart_putc(char data) {
    while (!(UCSR0A & (1<<UDRE0))); // Wait untill USART data register is empty
	UDR0 = data;
}

char usart_getc(void)
{
    while (!(UCSR0A & (1<<RXC0)));   
    return UDR0;                   
}

void usart_putbyte(uint8_t val) {
	char ch;
	uint8_t nibble = (val & 0xf0)>>4;
	if(nibble < 10 && nibble > 0)
		ch='0'+nibble;
	else if(nibble >=10)
			ch='a'+nibble-10;
		else
			ch='0';

	usart_putc(ch);


	nibble = val & 0xf;

	if(nibble < 10 && nibble > 0)
		ch='0'+nibble;
	else if(nibble >=10)
			ch='a'+nibble-10;
		else
			ch='0';

	usart_putc(ch);

}

void usart_puts(char *data) {
  int len, count;
  
  len = strlen(data);
  for (count = 0; count < len; count++) 
    usart_putc(*(data+count));
}

void usart_putint (int usart_num)				/* Writes integer to the USART		*/
{
 char usart_ascii_string[6]="      ";		/* Reservation to the max absvalue (2^16)/2		*/
 
 itoa(usart_num, usart_ascii_string, 10);	/* Convert integer to ascii [not ansi c]		*/
 usart_puts(usart_ascii_string);		/* Sens ascii pointer to the lcd_printf()		*/
}

void usart_putu32hex(u32 num)
{
	char s[10];
	usart_puts( ltoa(num,s,16) );

}


