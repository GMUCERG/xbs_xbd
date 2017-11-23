/*
 * XBD_HAL.c
 *
 *  Created on: Apr 9, 2017
 *      Author: ryan
 */

/* DriverLib Includes */
#include "driverlib.h"
#include "XBD_HAL.h"
#include <XBD_FRW.h>
#include "i2c_comms.h"

char * print_ptr;
unsigned int integer_print;
// Set optimization to 0 so this doesn't get removed by compiler
int __attribute__((optimize("O0")))
printf_gdb(char *str)
{
	print_ptr = str;
	return 0;
}

int __attribute__((optimize("O0")))
print_int_gdb(unsigned int print_int)
{
	integer_print = print_int;
	return 0;
}

void __attribute__((optimize("O0")))
printf_xbd(char *str)
{
	print_ptr = str;
	printf_gdb(str);
}

int __attribute__((optimize("O0")))
print_int_xbd(unsigned int print_int)
{
	integer_print = print_int;
	print_int_gdb(print_int);
	return 0;
}

void memcpy(void *dst, void *src, uint32_t size)
{
	unsigned int i = 0;
	for (i = 0; i < size; i++) {
		((char *)dst)[i] = ((char *)src)[i];
	}
}

void strcpy(char *dst, const char *src)
{
	unsigned int i = 0;
	for (i = 0; ((char *)src)[i] != 0; i++) {
		((char *)dst)[i] = ((char *)src)[i];
	}
	((char *)dst)[i] = 0;
}


void XBD_init() {
	printf_xbd("XBD_init\n");
	/* Halting the Watchdog */
	MAP_WDT_A_holdTimer();

	MAP_CS_setDCOCenteredFrequency(CS_DCO_FREQUENCY_12);
	MAP_CS_initClockSignal(CS_MCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1 );
	MAP_CS_initClockSignal(CS_HSMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1 );
	MAP_CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1 );
	MAP_CS_initClockSignal(CS_ACLK, CS_REFOCLK_SELECT, CS_CLOCK_DIVIDER_1);


	/* Configuring GPIO as an output */
	MAP_GPIO_setAsOutputPin(GPIO_PORT_P3, GPIO_PIN0);
	MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN0);
    
	/* Enabling MASTER interrupts */
	MAP_Interrupt_enableMaster();  

	i2c_set_rx(FRW_msgRecHand);
	i2c_set_tx(FRW_msgTraHand);
	i2c_init();
}

void XBD_serveCommunication(void) {
	i2c_rx();
}

void SysTick_Handler(void)
{
	
}


void XBD_loadStringFromConstDataArea( char *dst, const char *src  ) {
  /* copy a zero terminated string from src (CONSTDATAAREA) to dst
  e.g. strcpy if CONSTDATAREA empty, strcpy_P for PROGMEM
  */
  strcpy(dst,src);
}

void XBD_readPage( uint32_t pageStartAddress, uint8_t * buf ) {
  /* read PAGESIZE bytes from the binary buffer, index pageStartAddress
  to buf */
  memcpy(buf,(uint8_t *)pageStartAddress,PAGESIZE);
}

void XBD_programPage(uint32_t pageStartAddress, uint8_t *buf) {
	
}

inline void XBD_sendExecutionStartSignal() {
	MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN0);
}

inline void XBD_sendExecutionCompleteSignal() {
	MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN0);
}

void soft_reset(void) {
	
}

void XBD_switchToBootLoader(void) {
	
}

void XBD_switchToApplication() {

}

uint32_t XBD_busyLoopWithTiming(uint32_t approxCycles) {
	volatile uint32_t exactCycles;
	//printf_xbd("starting timing");
	
 	MAP_SysTick_disableModule();
	MAP_SysTick_disableInterrupt();
	MAP_SysTick_setPeriod(approxCycles+10000);
	//HWREG(NVIC_ST_CURRENT)=0;
	SysTick->VAL = 0;

	MAP_SysTick_enableModule();
	XBD_sendExecutionStartSignal();

	while( (exactCycles=MAP_SysTick_getValue()) > 10000);

	MAP_SysTick_disableModule();
	XBD_sendExecutionCompleteSignal();

	exactCycles = (approxCycles+10000)-MAP_SysTick_getValue();
	return exactCycles;
}

