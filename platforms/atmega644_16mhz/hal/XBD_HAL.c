#include <XBD_HAL.h>
#include <XBD_FRW.h>

#include <RS232.h>

#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/boot.h>
#include <util/delay_basic.h>
#include <i2c.h>
#include <alloca.h>

#define I2C_BAUDRATE 400
#define SLAVE_ADDR 1

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

	usart_init(115200);
	XBD_debugOut("START ATmega644 HAL\r\n");
	XBD_debugOut("\r\n");		 
	
	i2cInit();
	i2cSetLocalDeviceAddr(SLAVE_ADDR, 0);
	i2cSetBitrate(I2C_BAUDRATE);
	i2cSetSlaveReceiveHandler( FRW_msgRecHand ); 
	i2cSetSlaveTransmitHandler( FRW_msgTraHand );
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


void XBD_debugOut(char *message)
{
	usart_puts(message);	
}


void XBD_serveCommunication()
{
	if(inb(TWCR)&BV(TWINT)) {
		twi_isr();
	} 
}

#include "XBD_shared_HAL.i"
