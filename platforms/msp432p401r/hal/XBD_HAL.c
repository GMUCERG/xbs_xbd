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

#define STACK_CANARY (inv_sc?0x3A:0xC5)

extern uint32_t _end;

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

#ifdef BL
void SysTick_Handler(void) {

}
#endif

void XBD_serveCommunication(void) {
	i2c_rx();
}

void XBD_stopWatchDog(void)
{
	MAP_WDT_A_holdTimer();
}

void XBD_startWatchDog(uint32_t seconds)
{

}

void XBD_debugOut(const char *message) {
#ifdef DEBUG
	printf_xbd(message);
#endif
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

char sector_buf[4096];
void XBD_programPage(uint32_t pageStartAddress, uint8_t *buf) {
	unsigned int bank = FLASH_MAIN_MEMORY_SPACE_BANK0;
	unsigned int sector = pageStartAddress & 0xfffff000;
	unsigned int sector_start = sector;
	if (pageStartAddress >= 0x20000) {
		bank = FLASH_MAIN_MEMORY_SPACE_BANK1;
		sector -= 0x20000;
	}

	memcpy(sector_buf, (char *)sector_start, 4096);
	memcpy(&sector_buf[pageStartAddress - sector_start], buf, PAGESIZE);

	unsigned int sector_bits = 1 << (sector / 4096);

	MAP_FlashCtl_unprotectSector(bank, sector_bits);

	MAP_FlashCtl_eraseSector(pageStartAddress);
	MAP_FlashCtl_programMemory(sector_buf,
            (void*) sector_start, 4096);

	MAP_FlashCtl_protectSector(bank, sector_bits);
}

inline void XBD_sendExecutionStartSignal() {
	MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN0);
}

inline void XBD_sendExecutionCompleteSignal() {
	MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN0);
}

 __attribute__ ( ( naked ) )
void XBD_switchToApplication() {
  /* execute the code in the binary buffer */
  // __MSR_MSP(*(unsigned long *)0x2000);
   //((void(*)(void))FLASH_ADDR_MIN)();
   asm("mov pc,%[addr]"::[addr] "r" (FLASH_ADDR_MIN));
}

void soft_reset(void){
    MAP_ResetCtl_initiateSoftReset();
}

void XBD_switchToBootLoader() {
    MAP_ResetCtl_initiateSoftReset();
}

uint32_t XBD_busyLoopWithTiming(uint32_t approxCycles) {
	volatile uint32_t exactCycles;
	//printf_xbd("starting timing");
	
 	MAP_SysTick_disableModule();
	MAP_SysTick_disableInterrupt();
	MAP_SysTick_setPeriod(approxCycles+10000);
	SysTick->VAL = 0;

	MAP_SysTick_enableModule();
	XBD_sendExecutionStartSignal();

	while( (exactCycles=MAP_SysTick_getValue()) > 10000);

	MAP_SysTick_disableModule();
	XBD_sendExecutionCompleteSignal();

	exactCycles = (approxCycles+10000)-MAP_SysTick_getValue();
	return exactCycles;
}


void XBD_delayCycles(uint32_t approxCycles) {
	volatile uint32_t exactCycles;

 	MAP_SysTick_disableModule();
	MAP_SysTick_disableInterrupt();
	MAP_SysTick_setPeriod(approxCycles+10000);
	SysTick->VAL = 0;

	MAP_SysTick_enableModule();

	while( (exactCycles=MAP_SysTick_getValue()) > 10000);

	MAP_SysTick_disableModule();

	return;
}

volatile uint8_t *p_stack = 0;
uint8_t *p = 0;
uint8_t inv_sc=0;


__attribute__ ( ( noinline ) )
void getSP(volatile uint8_t **var_SP)
{
  volatile uint8_t beacon;
  *var_SP=&beacon;
  return;
}

uint32_t XBD_countStack(void) {
/* return stack measurement result */
    p = (uint8_t *)&_end;
    register uint32_t c = 0;

    while(*p == STACK_CANARY && p <= p_stack)
    {
        ++p;
        ++c;
    }

    return (uint32_t)c;
}

void XBD_paintStack(void) {
    //register void * __stackptr asm("sp");   ///<Access to the stack pointer
/* initialise stack measurement */
	p = (uint8_t *)&_end; 
	inv_sc=!inv_sc;
	getSP(&p_stack);
    //p_stack = __stackptr;

    
	while(p < p_stack)
        {
        *p = STACK_CANARY;
        ++p;
        } 
}

