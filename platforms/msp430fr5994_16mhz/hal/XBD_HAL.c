#include <msp430.h>   /* msp430-gcc */
#include <XBD_HAL.h>
#include <XBD_FRW.h>
#include <XBD_debug.h>
#include <stdint.h>
//#include <signal.h>
#include <string.h>


/* your functions / global variables here */
#include "i2c.h"
#include "RS232.h"
#include "clock_init.h"
//#include <JTAGfunc.h>

//#define DEBUG  // now in Makefile 

#define I2C_BAUDRATE 400
#define SLAVE_ADDR 0x75


#define STACK_CANARY (inv_sc?0x3A:0xC5)

//#pragma segment = "CSTACK"
uint8_t  inv_sc = 0;
static uint8_t *address;

//extern uint8_t __stack;		/* stack top address */
extern uint8_t _end;  ///<Last used byte of the last segment in RAM (defined by the linker)

//Cycle Count timer variable
volatile uint32_t TIMER_COUNT = 0;


void XBD_init() {

	__dint();
	WDTCTL = WDTPW|WDTHOLD;                   // Stop WDT
	// PM5CTL0 &= ~LOCKLPM5;          			// clear lock bit for ports


#ifdef BOOTLOADER

	initClocks();
#endif

//	P3SEL1 |= BIT4;				//SMCLK
//	P3DIR |= BIT4;

        
	// P5SEL1 |= BIT7;				//MCLK
	// P5SEL0 |= BIT7;
	// P5DIR |= BIT7;

#ifdef DEBUG
    //DEBUG LED(RED) - comment out this code when running tests
    P1OUT &= ~BIT0;                         
    P1DIR |= BIT0;              // Set P1.0 to output direction
    P1OUT |= BIT0;	   
    
    usart_init();
     
        
    XBD_debugOut("START MSP430FR5994 HAL\r\n");
	XBD_debugOut("\r\n");

	// Interrupt enable for global FRAM memory protection
	FRCTL0 = FRCTLPW;
    GCCTL0 |= WPIE;

			// Turn on led to indicate usart debug enabled
#else
    //DEBUG LED(RED) - OFF
    P1OUT &= ~BIT0;                         
    P1DIR |= BIT0;              // Set P1.0 to output direction
    P1OUT &= ~BIT0;				// Turn on led to indicate usart debug enabled

#endif

	i2cInit();
	i2cSetLocalDeviceAddr(SLAVE_ADDR, 0);
	//i2cSetBitrate(I2C_BAUDRATE);  // Slave, does not need to be set.
	i2cSetSlaveReceiveHandler( FRW_msgRecHand );
	i2cSetSlaveTransmitHandler( FRW_msgTraHand );

//    //Set Port 7.3 to output - Execution signal to XBH
	P5SEL1 &= ~BIT7;
	P5SEL0 &= ~BIT7;
	P5DIR |= BIT7;
	P5OUT |= BIT7;          // Set pin high

	// Now we are having hard interrupt from XBH
	//Enable interrupt for soft reset on pin 6.3
	//P6SEL1 &= ~BIT3;
	//P6SEL0 &= ~BIT3;
	//P6DIR &= ~BIT3;
	//P6IES &= ~BIT3;         //Trigger on rising edge
	//P6IE = BIT3;

	__eint();

	// XBD_switchToApplication();
}


inline void XBD_sendExecutionStartSignal() {
	P5OUT &= (~BIT7);
	/* code for output pin = on here */
}

inline void XBD_sendExecutionCompleteSignal() {
	/* code for output pin = off here */
	P5OUT |= BIT7;
}


void XBD_debugOut(const char *message) {
#ifdef DEBUG
	 //if you have some kind of debug interface, write message to it 
	usart_puts(message);
#endif
#ifdef XBX_DEBUG_APP
	 //if you have some kind of debug interface, write message to it 
	usart_puts(message);
#endif
}

void XBD_serveCommunication() {
	/* read from XBD<->XBH communication channel (uart or i2c) here and call

	   FRW_msgTraHand(size,transmitBuffer)
	   FRW_msgRecHand(size,transmitBuffer)

	   depending whether it's a read or write request
	   */
 
	//if(UCB0STAT&UCSTTIFG) {
	//if(((IFG2&UCB0RXIFG)== 1) || ((IFG2&UCB0TXIFG) == 1)) {
	  twi_isr();
	//}
}

void XBD_loadStringFromConstDataArea( char *dst, const char *src  ) {
	strcpy((char *)dst,(const char *)src);
}


void XBD_readPage( uint32_t pageStartAddress, uint8_t * buf ) {
	/* read PAGESIZE bytes from the binary buffer, index pageStartAddress
	   to buf */
	uint8_t *startAddress = (uint8_t *) (uint16_t) pageStartAddress;

	uint16_t u;
	for(u = 0;u < 256; ++u)			//may need to change to 128
	{
		*buf++ =  *startAddress++;
	}
}

void XBD_programPage( uint32_t pageStartAddress, uint8_t * buf ) {
	/* copy data from buf (PAGESIZE bytes) to pageStartAddress of
	   application binary */

	//	Interrupts disabled in XBD_init()
	uint16_t u;
	uint16_t *startAddress = (uint16_t *) (uint16_t) pageStartAddress;
	uint16_t *bufPtr = (uint16_t *)buf;

// 	__disable_interrupt();
// 	//	Erase a page of flash
// 	//FCTL2 = FWKEY|FSSEL1|FN0; // Set clock divider to 1
// 	FCTL3 = FWKEY;            // Unlock
// 	FCTL1 = FWKEY|ERASE;      // Set erase bit
// 	*startAddress = 0x00;     // Perform erase
// 	// Waiting until erase and clearing bit should be unnecessary
// 	// when erasing from program running from flash
// #if 0
// 	while(FCTL3&BUSY);        // Wait until erase done
// 	FCTL1 = FWKEY;            // Clear erase bit
// #endif
// 	//Disabling watchdog should be unnecessary, as it should be disabled already
// #if 0
// 	WDTCTL = WDTPW+WDTHOLD;            // Stop WDT
// #endif

// 	//for(i=0;i<1000; i++)
// 	//    asm("nop");
// 	////WDTCTL = WDTPW+0x0c;

// 	//	Write to flash
// 	startAddress = (uint16_t *) (uint16_t) pageStartAddress; // 16-bits
// 	while(FCTL3&BUSY);                                      // Wait until flash ready
// 	// FCTL3 = FWKEY;                                        // Clear lock bits (LOCK & LOCKA)
// 	FCTL1 = FWKEY | WRT;                                     // Enable byte/word write mode
// 	for(u = 0;u < PAGESIZE/(sizeof(uint16_t)); u++)          // may need to change to 128
// 	{
// 	*startAddress++ = *bufPtr++;
// 	}
// 	FCTL1 = FWKEY;                                           // Clear write bit
// 	FCTL3 = FWKEY|LOCK;                                      // Lock flash
// 	__enable_interrupt();

	// XBD_debugOut("In function XBD_programPage\r\n");

	// new CODE
	__disable_interrupt();
	FRCTL0 = FRCTLPW;				//unlock write to registers, otherwise generates PUC, later lock in byte mode
	// FRCTL0 &= ~WPROT;				//write only to lower byte?
	for(u = 0;u < PAGESIZE/(sizeof(uint16_t)); u++)          
	{
		*startAddress++ = *bufPtr++;


		// usart_putint(startAddress);
		// XBD_debugOut("\r\n");
		if(*(startAddress-1)!= *(bufPtr-1)){
			XBD_debugOut("Data not properly written to FRAM\r\n");
		}
	}

	// FRCTL0_H = 0;				//relock write access to registers
	__enable_interrupt();
	// XBD_debugOut("Going out of function XBD_programPage\r\n");








}

void XBD_switchToApplication() {
	/* execute the code in the binary buffer */
	// pointer called reboot that points to the reset vector
	XBD_debugOut("Going to application mode\r\n");
	//usart_putint(FLASH_ADDR_MIN);
	XBD_debugOut("\r\n");	

	//void (*reboot)( void ) = (void*)FLASH_ADDR_MIN; // defines the function reboot to location 0x3200
	void (*reboot)( void ) = (void*) 0x08000; // defines the function reboot to location 0x3200
	
	reboot();	// calls function reboot function

	XBD_debugOut("Returned from application mode\r\n");
}


void XBD_switchToBootLoader() {
   	XBD_debugOut("Soft Reset to switch to Bootloader\r\n");
	WDTCTL = 0;     //Write invalid key to trigger soft reset
					//XXX Be aware soft reset does not reset register values, thus
					//code must initialize registers before use.
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
__attribute__((optimize("no-strict-aliasing")))
/**
 */
uint32_t XBD_busyLoopWithTiming(volatile uint32_t approxCycles) {
	/* wait for approximatly approxCycles,
	 * then return the number of cycles actually waited */
	/* Code from atmega_common XBD_shared_HAL.i */
	// uint32_t exactCycles=0;
	//Macro to cast registers to larger type since thye have adjacent addresses.
// #define RTCNT (*((volatile uint32_t*) &RTCNT1))
	//uint16_t lastTCNT=0;
	//uint16_t overRuns=0; //Overruns not necessary, 32 bit counter

	// __disable_interrupt();
	// RTCCTL01 = RTCHOLD | RTCSSEL0| RTCTEV1|RTCTEV0;
	// RTCNT = 0;  
	// XBD_sendExecutionStartSignal();
	// RTCCTL01 &= ~RTCHOLD; //Start clock
	// while(RTCNT < approxCycles);
	// RTCCTL01 |= RTCHOLD; //Stop clock
	// XBD_sendExecutionCompleteSignal();
	// exactCycles = RTCNT;
	// __enable_interrupt();

	// return exactCycles;

	//new CODE RTC - doesn't work because RTC can't source from SMCLK. Use Timers.
// 	uint32_t exactCycles=0;
// #define RTCNT (*((volatile uint32_t*) &RTCCNT1))

// 	__disable_interrupt();
// 	RTCCTL0_H = RTCKEY_H;                      //unlock RTC_C for writes
// 	RTCCTL13 = RTCHOLD | RTCSSEL0 | RTCTEV1|RTCTEV0;
// 	RTCNT = 0;                               //reset ctr
// 	XBD_sendExecutionStartSignal();
// 	RTCCTL13 &= ~RTCHOLD;
// 	while(RTCNT < approxCycles);
// 	RTCCTL13 |= RTCHOLD;
// 	XBD_sendExecutionCompleteSignal();
// 	exactCycles = RTCNT;
// 	RTCCTL0_H = 0;      					//lock RTC_C again
// 	__enable_interrupt();

// 	return exactCycles;

	//Timer Code
	uint32_t exactCycles=0;
	TIMER_COUNT = 0;						//Reset Timer

	// SMCLK, contmode, clear TAR, enable overflow interrupt
	TA0CTL = TASSEL__SMCLK | MC__CONTINUOUS | TACLR | TAIE;		//START CLOCK
	XBD_sendExecutionStartSignal();
	__enable_interrupt();
	// __delay_cycles(approxCycles);
	__delay_cycles(F_CPU);			//Use #if F_CPU to set approx cycles (16x2^15,8x2^15,...)
	__disable_interrupt();
	XBD_sendExecutionCompleteSignal();
	TA0CTL |= MC__STOP;						//STOP CLOCK
	TIMER_COUNT += TA0R;					//needs to be atomic
	__enable_interrupt();

	exactCycles = TIMER_COUNT;
	TIMER_COUNT = 0;						//Reset Timer

	return exactCycles;

}
#pragma GCC diagnostic pop

void XBD_paintStack(void) {
	/* initialise stack measurement */

	inv_sc=!inv_sc;

	//uint8_t *p = &__stack;		/* p = address of stack top */
	//uint8_t *p = __segment_begin("CSTACK");//(void *)0x1100;     //__segment_begin("CSTACK");
	//get address
	//uint8_t *p_stack = __segment_end("CSTACK");

	address = &_end;
	register void * __stackptr asm("1");   ///<Access to the stack pointer
	while(address <  ((uint8_t *)__stackptr))
	{
		*address = STACK_CANARY;
		++address;
	}
}

uint32_t XBD_countStack(void) {
	/* return stack measurement result/0 if not supported */
	//uint8_t *p = __segment_end("CSTACK");
	//uint8_t *p_stack = __segment_begin("CSTACK");
	address = &_end;
	register void * __stackptr asm("1");   ///<Access to the stack pointer
	register uint16_t c = 0;
	while(*address == STACK_CANARY && address <= (uint8_t*)__stackptr)
	{
		++address;
		++c;
	}
	return (uint32_t)c;
}


//NOPs, since we can do external reset
void XBD_startWatchDog(uint32_t seconds)
{
}

void XBD_stopWatchDog()
{
}

#ifdef BOOTLOADER
/**
 * Soft reset using interrupt lines from XBH
 */
__attribute__((used,interrupt(PORT6_VECTOR))) void soft_reset(void){
	P6IFG &= ~BIT3; //Clear IFG for pin 2.0
	if(!(P6IN&BIT3)){
		WDTCTL = 0;     //Write invalid key to trigger soft reset
						//XXX Be aware soft reset does not reset register values, thus
						//code must initialize registers before use.
	}

	
	//(WDTPW+WDTTMSEL+WDTCNTCL+WDTIS1+WDTIS0) Soft reset after 0.064ms at 1MHz

	//software reset (this is a soft reset equivalent on msp430 at 32ms 1Mhz smclk)
	//  wdtctl = WDT_MDLY_32; //wdt_enable(WDTO_15MS); //set watch dog WDT_MDLY_32
	return;
}

#endif


// Timer0_A1 Interrupt Vector (TAIV) handler - Cycle Counting
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=TIMER0_A1_VECTOR
__interrupt void TIMER0_A1_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(TIMER0_A1_VECTOR))) TIMER0_A1_ISR (void)
#else
#error Compiler not supported!
#endif
{
    switch(__even_in_range(TA0IV, TAIV__TAIFG))
    {
        // case TAIV__NONE:   break;           // No interrupt
        // case TAIV__TACCR1: break;           // CCR1 not used
        // case TAIV__TACCR2: break;           // CCR2 not used
        // case TAIV__TACCR3: break;           // reserved
        // case TAIV__TACCR4: break;           // reserved
        // case TAIV__TACCR5: break;           // reserved
//      //  case TAIV__TACCR6: break;           // reserved
        case TAIV__TAIFG:                   // overflow
        	TIMER_COUNT += 0xffff;
        	TA0CTL &= ~TAIFG;		//clear interrupt
            break;
        default: break;
    }
}




// //SYSNMI - used for invaild writes to FRAM
// #if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
// #pragma vector=SYSNMI_VECTOR
// __interrupt void SYSNMI_ISR(void)
// #elif defined(__GNUC__)
// void __attribute__ ((interrupt(SYSNMI_VECTOR))) SYSNMI_ISR (void)
// #else
// #error Compiler not supported!
// #endif
// {
//     switch(__even_in_range(SYSSNIV, SYSSNIV__LEASCCMD))
//     {
//         case SYSSNIV__NONE       : break;   // No interrupt pending
//         case SYSSNIV__UBDIFG     : break;   // Uncorrectable FRAM bit error detection
//         case SYSSNIV__ACCTEIFG   : break;   // FRAM Access Time Error
//         case SYSSNIV__MPUSEGPIFG : break;   // MPUSEGPIFG encapsulated IP memory segment violation
//         case SYSSNIV__MPUSEGIIFG : break;   // MPUSEGIIFG information memory segment violation
//         case SYSSNIV__MPUSEG1IFG : break;   // MPUSEG1IFG segment 1 memory violation
//         case SYSSNIV__MPUSEG2IFG : break;   // MPUSEG2IFG segment 2 memory violation
//         case SYSSNIV__MPUSEG3IFG : break;   // MPUSEG3IFG segment 3 memory violation
//         case SYSSNIV__VMAIFG     : break;   // VMAIFG Vacant memory access
//         case SYSSNIV__JMBINIFG   : break;   // JMBINIFG JTAG mailbox input
//         case SYSSNIV__JMBOUTIFG  : break;   // JMBOUTIFG JTAG mailbox output
//         case SYSSNIV__CBDIFG     : break;   // Correctable FRAM bit error detection
//         case SYSSNIV__WPROT      :          // FRAM write protection detection
//             XBD_debugOut("DATA WRITTEN TO FRAM BUT 'WPROT' BIT NOT CLEARED\r\n");
//             break;
//         case SYSSNIV__LEASCTO    : break;   // LEA time-out fault
//         case SYSSNIV__LEASCCMD   : break;   // LEA command fault
//         default: break;
//     }
// }
