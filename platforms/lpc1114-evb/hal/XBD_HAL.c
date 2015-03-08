#include <XBD_HAL.h>
#include <XBD_FRW.h>
#include <XBD_debug.h>

#include <string.h>
/* device specific includes */


#include "projectconfig.h"
#include "lpc111x.h"
#include "gpio.h"
#include "cpu.h"
#include "wdt.h"
#include "iap.h"
#include "wdt.h"


/* your functions / global variables here */
#define STACK_CANARY (inv_sc?0x3A:0xC5)

void writeByte(unsigned char byte);
unsigned char readByte();

uint32_t XBD_busyLoopWithTiming(uint32_t approxCycles);

void blinkIt(int count)
{
  int i;
  for(i=0;i<count*2;++i)
  {
    gpioSetValue(CFG_LED_PORT,CFG_LED_PIN,i%2);
    XBD_busyLoopWithTiming(DEVICE_SPECIFIC_SANE_TC_VALUE/10);
  }  
}

void uartInit(uint32_t baudrate)
{
  uint32_t fDiv;
  uint32_t regVal;

  /* Set 1.6 UART RXD */
  IOCON_PIO1_6 &= ~IOCON_PIO1_6_FUNC_MASK;
  IOCON_PIO1_6 |= IOCON_PIO1_6_FUNC_UART_RXD;

  /* Set 1.7 UART TXD */
  IOCON_PIO1_7 &= ~IOCON_PIO1_7_FUNC_MASK;	
  IOCON_PIO1_7 |= IOCON_PIO1_7_FUNC_UART_TXD;

  /* Enable UART clock */
  SCB_SYSAHBCLKCTRL |= (SCB_SYSAHBCLKCTRL_UART);
  SCB_UARTCLKDIV = SCB_UARTCLKDIV_DIV1;     /* divided by 1 */

  /* 8 bits, no Parity, 1 Stop bit */
  UART_U0LCR = (UART_U0LCR_Word_Length_Select_8Chars |
                UART_U0LCR_Stop_Bit_Select_1Bits |
                UART_U0LCR_Parity_Disabled |
                UART_U0LCR_Parity_Select_OddParity |
                UART_U0LCR_Break_Control_Disabled |
                UART_U0LCR_Divisor_Latch_Access_Enabled);

  /* Baud rate */
  regVal = SCB_UARTCLKDIV;
  
  fDiv = (((CFG_CPU_CCLK * SCB_SYSAHBCLKDIV)/regVal)/16 + baudrate/2 )/baudrate;

  UART_U0DLM = fDiv / 256;
  UART_U0DLL = fDiv % 256;
  
  /* Set DLAB back to 0 */
  UART_U0LCR = (UART_U0LCR_Word_Length_Select_8Chars |
                UART_U0LCR_Stop_Bit_Select_1Bits |
                UART_U0LCR_Parity_Disabled |
                UART_U0LCR_Parity_Select_OddParity |
                UART_U0LCR_Break_Control_Disabled |
                UART_U0LCR_Divisor_Latch_Access_Disabled);
  
  /* Enable and reset TX and RX FIFO. */
  UART_U0FCR = (UART_U0FCR_FIFO_Enabled | 
                UART_U0FCR_Rx_FIFO_Reset | 
                UART_U0FCR_Tx_FIFO_Reset); 

  /* Read to clear the line status. */
  regVal = UART_U0LSR;

  /* Ensure a clean start, no data in either TX or RX FIFO. */
  while (( UART_U0LSR & (UART_U0LSR_THRE|UART_U0LSR_TEMT)) != (UART_U0LSR_THRE|UART_U0LSR_TEMT) );
  while ( UART_U0LSR & UART_U0LSR_RDR_DATA )
  {
    /* Dump data from RX FIFO */
    regVal = UART_U0RBR;
  }

  return;
}


void XBD_init()
{
  //NVIC: disable ALL interrupts
  *((unsigned int *)(0xE000E180)) = 0xffffffff;
  
  cpuInit();
  gpioInit();
  uartInit(250000);
  
  // Set LED pin as output and turn LED off
  gpioSetDir(CFG_LED_PORT, CFG_LED_PIN, gpioDirection_Output);
  gpioSetValue(CFG_LED_PORT, CFG_LED_PIN, CFG_LED_ON);

  gpioSetDir(CFG_XBX_PORT, CFG_XBX_PIN, gpioDirection_Output);
  gpioSetValue(CFG_XBX_PORT, CFG_XBX_PIN, CFG_LED_OFF);

#ifdef BOOTLOADER
  blinkIt(2);
#else
  blinkIt(6);
#endif

  writeByte('A');                 
}


inline void XBD_sendExecutionStartSignal()
{
  /* code for output pin = low here */
  gpioSetValue (CFG_XBX_PORT, CFG_XBX_PIN, CFG_LED_ON); 

}

inline void XBD_sendExecutionCompleteSignal()
{
  /* code for output pin = high here */
  gpioSetValue (CFG_XBX_PORT, CFG_XBX_PIN, CFG_LED_OFF);
}


void XBD_debugOut(const char *message)
{
  /* if you have some kind of debug interface, write message to it */
  (void)message;
}


/* read a byte from the serial port */
unsigned char readByte()
{
	//blinkIt(1);
	//check for RX'd byte available
	while( 0 == (*((volatile unsigned int *)(0x40008014)) & 0x1) );
	
	return (unsigned char)(*((unsigned int *)(0x40008000)));
}


/* write a byte to the serial port */
void writeByte(unsigned char byte)
{
	//wait for TX empty
	while( 0 == (*((volatile unsigned int *)(0x40008014)) & 0x40) );
	//put byte on top of the tx FIFO (whis is off anyway, i.e. fifo size=1)
	*((unsigned int *)(0x40008000)) = (unsigned int)byte;
}


#include <XBD_streamcomm.i>
/*
* XBD_streamcomm.i
* Provides emulation of TWI (I2C) over stream oriented channel (e.g. UART).
* May be included by HALs that want to use it, otherwise unused.
* Requires the HAL to provide
*
* unsigned char readByte();
* and
* void writeByte(unsigned char byte);
*
* (both blocking)
*/

void XBD_loadStringFromConstDataArea( char *dst, const char *src  ) {
  /* copy a zero terminated string from src (CONSTDATAAREA) to dst 
  e.g. strcpy if CONSTDATAREA empty, strcpy_P for PROGMEM
  */
  strcpy(dst,src);
}


void XBD_readPage( uint32_t pageStartAddress, uint8_t * buf )
{
  /* read PAGESIZE bytes from the binary buffer, index pageStartAddress
  to buf */
  memcpy(buf,(uint8_t *)pageStartAddress,PAGESIZE);
}

void XBD_programPage( uint32_t pageStartAddress, uint8_t * buf )
{
  /* copy data from buf (PAGESIZE bytes) to pageStartAddress of
  application binary */
  IAP_return_t ret;
  
  #define SECTOR_NUMBER (pageStartAddress/PAGESIZE)
  
  ret=iapPrepareSector(SECTOR_NUMBER);
  ret=iapEraseSector(SECTOR_NUMBER);
  ret=iapPrepareSector(SECTOR_NUMBER);
  ret=iapCopyRAMToFlash(pageStartAddress, (uint32_t) buf, PAGESIZE);
  
  blinkIt( ret.ReturnCode );
}

void XBD_switchToApplication()
{
  (*((void(*)(void))(*(unsigned long*)APP_START_ADDRESS)))();
}


void XBD_switchToBootLoader()
{
  /* switch back from uploaded code to boot loader */
  wdtInit();        
  // Pat the watchdog to start it
  wdtFeed();
  WDT_WDMOD = WDT_WDMOD_WDEN_ENABLED |
	    	  WDT_WDMOD_WDRESET_ENABLED;
  //intentionally feed wrong numbers to cause reset
  WDT_WDFEED = WDT_WDFEED_FEED1+1;
  WDT_WDFEED = WDT_WDFEED_FEED2+1;
  while(1){
  }
}

uint32_t XBD_busyLoopWithTiming(uint32_t approxCycles)
{
  /* wait for approximatly approxCycles, 
   * then return the number of cycles actually waited */

  volatile uint32_t exactCycles;
  
  SCB_SYSAHBCLKCTRL |= (SCB_SYSAHBCLKCTRL_CT32B0);
  TMR_TMR32B0PR = 0;	//count at full cpu clock

  TMR_TMR32B0TCR = TMR_TMR32B0TCR_COUNTERENABLE_ENABLED|TMR_TMR32B0TCR_COUNTERRESET_ENABLED;
  
  TMR_TMR32B0TCR = TMR_TMR32B0TCR_COUNTERENABLE_ENABLED;	//Let timer run!
  XBD_sendExecutionStartSignal();
  
  while( (exactCycles=TMR_TMR32B0TC) < approxCycles);
          
  XBD_sendExecutionCompleteSignal();
         
  return exactCycles; 
}

void XBD_delayCycles(uint32_t approxCycles)
{
  volatile uint32_t exactCycles;
  
  SCB_SYSAHBCLKCTRL |= (SCB_SYSAHBCLKCTRL_CT32B0);
  TMR_TMR32B0PR = 0;	//count at full cpu clock

  TMR_TMR32B0TCR = TMR_TMR32B0TCR_COUNTERENABLE_ENABLED|TMR_TMR32B0TCR_COUNTERRESET_ENABLED;
  
  TMR_TMR32B0TCR = TMR_TMR32B0TCR_COUNTERENABLE_ENABLED;	//Let timer run!
  
  while( (exactCycles=TMR_TMR32B0TC) < approxCycles);
    
  return;   
}



uint8_t *p_stack = LPC1114_STACKTOP;
uint8_t *p = LPC1114_STACKBOTTOM;
uint8_t inv_sc=0;


void getSP(uint8_t **var_SP)
{
  volatile uint8_t beacon;
  *var_SP=&beacon;
  return;
}

void XBD_paintStack(void)
{
/* initialise stack measurement */
	p = LPC1114_STACKBOTTOM;
	inv_sc=!inv_sc;
	getSP(&p_stack);

    
	while(p <= p_stack)
        {
        *p = STACK_CANARY;
        ++p;
        } 
}

uint32_t XBD_countStack(void)
{
/* return stack measurement result */
    p = LPC1114_STACKBOTTOM;
    register uint32_t c = 0;

    while(*p == STACK_CANARY && p <= p_stack)
    {
        ++p;
        ++c;
    }

    return (uint32_t)c;
} 

void XBD_startWatchDog(uint32_t seconds)
{
  wdtInit();        
  // Pat the watchdog to start it
  wdtFeed();
}
 
void XBD_stopWatchDog()
{
}
