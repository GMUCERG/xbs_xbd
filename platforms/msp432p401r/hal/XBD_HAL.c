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
	/* Halting the Watchdog */
	MAP_WDT_A_holdTimer();

	/* Configuring GPIO as an output */
	MAP_GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);

	/* Configuring SysTick to trigger at 1500000 (MCLK is 3MHz so this will make
	* it toggle every 0.5s) */
	MAP_SysTick_enableModule();
	MAP_SysTick_setPeriod(750000);
	//MAP_Interrupt_enableSleepOnIsrExit();
	MAP_SysTick_enableInterrupt();
    
	/* Enabling MASTER interrupts */
	MAP_Interrupt_enableMaster();  

	printf_xbd("Test Print\n");
}

void XBD_serveCommunication(void) {
	unsigned int i = 0;

	for (i = 0; i < 1500; i++) {
		//MAP_PCM_gotoLPM0();
	}	
	printf_xbd("Test Print\n");
}

void SysTick_Handler(void)
{
    MAP_GPIO_toggleOutputOnPin(GPIO_PORT_P1, GPIO_PIN0);
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
	
}

inline void XBD_sendExecutionCompleteSignal() {
	
}

void soft_reset(void) {
	
}

void XBD_switchToBootLoader(void) {
	
}

void XBD_switchToApplication() {

}

uint32_t XBD_busyLoopWithTiming(uint32_t approxCycles) {
	return 0;
}

