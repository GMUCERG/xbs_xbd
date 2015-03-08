#include <XBD_HAL.h>
#include <XBD_FRW.h>

#include <RS232.h>

#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/boot.h>
#include <util/delay_basic.h>
#include <alloca.h>

uint8_t mcusr_mirror __attribute__ ((section (".noinit")));

void get_mcusr(void) \
  __attribute__((naked)) \
  __attribute__((section(".init3")));
void get_mcusr(void)
{
  mcusr_mirror = MCUSR;
  MCUSR = 0;
  wdt_disable();
}


void XBD_init()
{
 	cli();               // Disable interrupts globally

	usart_init(250000);
	usart_putc('A');	// ACK any pending UART communication from XBH
	XBD_debugOut("START ATmega1284P HAL\r\n");
	XBD_debugOut("\r\n");		 

	PORTD |= _BV(PD4);	//PD4 to output high -> waiting 4 EXr
	DDRD |= _BV(DDD4);
}


inline void XBD_sendExecutionStartSignal()
{
	PORTD &= ~(_BV(PD4));	//falling edge for ICP on XBH
}


inline void XBD_sendExecutionCompleteSignal()
{
	PORTD |= _BV(PD4);	//rising edge for ICP on XBH
}


//volatile char *p_monitor;
void XBD_debugOut(const char *message)
{
	//p_monitor=message;
	//p_monitor++;
	//usart_puts(message);	
}


unsigned char readByte()
{
	return (unsigned char)usart_getc();
}

void writeByte(unsigned char byte)
{
	//volatile unsigned char lookibyte=byte;
	//if(lookibyte > 250) usart_putc('c');
	usart_putc((char)byte);
	//_delay_us(120);
}

#include <XBD_streamcomm.i>
/*
* XBD_streamcomm.i
* Provides emulation of TWI (I²C) over stream oriented channel (e.g. UART).
* May be included by HALs that want to use it, otherwise unused.
* Requires the HAL to provide
*
* unsigned char readByte();
* and
* void writeByte(unsigned char byte);
*
* (both blocking)
*/

/** rs232 transmit buffer 
uint32_t aligned_transmitBuffer[256/sizeof(uint32_t)];
uint8_t  *transmitBuffer=(uint8_t *)aligned_transmitBuffer;
*/
/*
volatile char vc;


void dummy()
{
	vc++;
	vc--;
	vc++;
	vc--;
	vc++;
	vc--;
}

void XBD_serveCommunication()
{
  volatile unsigned char byte;
  
  // scan for preamble, try again later if not found 
  byte=readByte();
  if(byte!=0) return;
  byte=readByte();
  if(byte!=0) return;
  byte=readByte();
  // read away rest of preamble 
  while(byte==0) byte=readByte();


  unsigned char requestType=byte;
  volatile unsigned char size=readByte();
  int sendSize;
  int i;
  
//	dummy();
  if(size > 7 && requestType=='R')
  {
//	dummy();
  }
  if(requestType=='R') {
   // XBD_debugOut("Received read request of size ");
   // XBD_debugOutHexByte(size);
   // XBD_debugOut("\n");
    sendSize=FRW_msgTraHand(size,transmitBuffer);
   // XBD_debugOut("Send size is ");
   // XBD_debugOutHexByte(sendSize);
   // XBD_debugOut("\n");
   // XBD_debugOutBuffer("Send buffer",transmitBuffer,size);
    for(i=0; i< size;i++) writeByte(transmitBuffer[i]);
  } else if(requestType=='W') {
   // XBD_debugOut("Received write request of size ");
   // XBD_debugOutHexByte(size);
   // XBD_debugOut("\n");
    for(i=0;i<size;i++) {
      transmitBuffer[i]=readByte();
    }
    FRW_msgRecHand(size,transmitBuffer);
	writeByte('A');
  }
}
*/

#include "XBD_shared_HAL.i"
