#include <XBD_HAL.h>
#include <XBD_FRW.h>
#include <XBD_debug.h>

/* your functions / global variables here */
#include "signal.h"


void XBD_init()
{
  /* inititalisation code, called once */
}


inline void XBD_sendExecutionStartSignal()
{
  /* code for output pin = on here */

}

inline void XBD_sendExecutionCompleteSignal()
{
  /* code for output pin = off here */
}


void XBD_debugOut(char *message)
{
  /* if you have some kind of debug interface, write message to it */
	usart_puts(message);
}

void XBD_serveCommunication()
{
  /* read from XBD<->XBH communication channel (uart or i2c) here and call

    FRW_msgTraHand(size,transmitBuffer)
    FRW_msgRecHand(size,transmitBuffer)
    
    depending whether it's a read or write request 
  */
  	if((IFG2&UCB0RXIFG== 1) | (IFG2&UCB0TXIFG == 1)) {
		twi_isr();
	}
}

void XBD_loadStringFromConstDataArea( char *dst, const char *src  ) {
  /* copy a zero terminated string from src (CONSTDATAAREA) to dst 
  e.g. strcpy if CONSTDATAREA empty, strcpy_P for PROGMEM
  */
}


void XBD_readPage( uint32_t pageStartAddress, uint8_t * buf )
{
  /* read PAGESIZE bytes from the binary buffer, index pageStartAddress
  to buf */
}

void XBD_programPage( uint32_t pageStartAddress, uint8_t * buf )
{
  /* copy data from buf (PAGESIZE bytes) to pageStartAddress of
  application binary */
}

void XBD_switchToApplication()
{
  /* execute the code in the binary buffer */
}


void XBD_switchToBootLoader()
{
  /* switch back from uploaded code to boot loader */
  _RESET(void);
}

uint32_t XBD_busyLoopWithTiming(uint32_t approxCycles)
{
  /* wait for approximatly approxCycles, 
  * then return the number of cycles actually waited */
}



void XBD_paintStack(void)
{
/* initialise stack measurement */
}

uint32_t XBD_countStack(void)
{
/* return stack measurement result/0 if not supported */
  return 0;
}
