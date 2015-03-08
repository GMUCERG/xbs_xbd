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

#define I2C_BAUDRATE 400
#define SLAVE_ADDR 0x75


#define STACK_CANARY (inv_sc?0x3A:0xC5)

//#pragma segment = "CSTACK"
uint8_t  inv_sc = 0;
static uint8_t *address;

//extern uint8_t __stack;		/* stack top address */
extern uint8_t _end;  ///<Last used byte of the last segment in RAM (defined by the linker)


void XBD_init() {

    __dint();
    WDTCTL = WDTPW|WDTHOLD;                   // Stop WDT

#ifdef BOOTLOADER
    initClocks();
#endif

#ifdef DEBUG
    usart_init();
    XBD_debugOut("START MSP430F5529 HAL\r\n");
    XBD_debugOut("\r\n");
#endif

    i2cInit();
    i2cSetLocalDeviceAddr(SLAVE_ADDR, 0);
    //i2cSetBitrate(I2C_BAUDRATE);  // Slave, does not need to be set.
    i2cSetSlaveReceiveHandler( FRW_msgRecHand );
    i2cSetSlaveTransmitHandler( FRW_msgTraHand );

    //Set Port 2.0 to output - Execution signal to XBH
    P1SEL &= ~BIT3;
    P1DIR |= BIT3;
    P1OUT |= BIT3;     // Set pin high

    //Enable interrupt for soft reset on pin 2.0
    P2SEL &= ~BIT7;
    P2DIR &= ~BIT7;
    P2IES |= BIT7; //Trigger on falling edge
    P2IE = BIT7;

    __eint();
}


inline void XBD_sendExecutionStartSignal() {
    P1OUT &= (~BIT3);
    /* code for output pin = on here */
}

inline void XBD_sendExecutionCompleteSignal() {
    /* code for output pin = off here */
    P1OUT |= BIT3;
}


void XBD_debugOut(const char *message) {
#ifdef DEBUG
    /* if you have some kind of debug interface, write message to it */
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

    __disable_interrupt();
    //	Erase a page of flash
    //FCTL2 = FWKEY|FSSEL1|FN0; // Set clock divider to 1
    FCTL3 = FWKEY;            // Unlock
    FCTL1 = FWKEY|ERASE;      // Set erase bit
    *startAddress = 0x00;     // Perform erase
    // Waiting until erase and clearing bit should be unnecessary
    // when erasing from program running from flash
#if 0
    while(FCTL3&BUSY);        // Wait until erase done
    FCTL1 = FWKEY;            // Clear erase bit
#endif
    //Disabling watchdog should be unnecessary, as it should be disabled already
#if 0
    WDTCTL = WDTPW+WDTHOLD;            // Stop WDT
#endif

    //for(i=0;i<1000; i++)
    //    asm("nop");
    ////WDTCTL = WDTPW+0x0c;

    //	Write to flash
    startAddress = (uint16_t *) (uint16_t) pageStartAddress; // 16-bits
    while(FCTL3&BUSY);                                      // Wait until flash ready
    // FCTL3 = FWKEY;                                        // Clear lock bits (LOCK & LOCKA)
    FCTL1 = FWKEY | WRT;                                     // Enable byte/word write mode
    for(u = 0;u < PAGESIZE/(sizeof(uint16_t)); u++)          // may need to change to 128
    {
    *startAddress++ = *bufPtr++;
    }
    FCTL1 = FWKEY;                                           // Clear write bit
    FCTL3 = FWKEY|LOCK;                                      // Lock flash
    __enable_interrupt();
}

void XBD_switchToApplication() {
    /* execute the code in the binary buffer */
    // pointer called reboot that points to the reset vector
    // bootloader is located at 0xe000
    void (*reboot)( void ) = (void*)FLASH_ADDR_MIN; // defines the function reboot to location 0x3200
    reboot();	// calls function reboot function
}


void XBD_switchToBootLoader() {
    /* execute the code in the binary buffer */
    // pointer called reboot that points to the reset vector
    // app is located at 0x3200
    void (*reboot)( void ) = (void*)(FLASH_ADDR_MAX+1); // defines the function reboot to location 0xe000
    reboot();	// calls function reboot function, did not need to change unless change location to 0x1000, at flash info memory
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
    uint32_t exactCycles=0;
    //Macro to cast registers to larger type since thye have adjacent addresses.
#define RTCNT (*((volatile uint32_t*) &RTCNT1))
    //uint16_t lastTCNT=0;
    //uint16_t overRuns=0; //Overruns not necessary, 32 bit counter

    __disable_interrupt();
    RTCCTL01 = RTCHOLD | RTCSSEL0| RTCTEV1|RTCTEV0;
    RTCNT = 0;
    XBD_sendExecutionStartSignal();
    RTCCTL01 &= ~RTCHOLD; //Start clock
    while(RTCNT < approxCycles);
    RTCCTL01 |= RTCHOLD; //Stop clock
    XBD_sendExecutionCompleteSignal();
    exactCycles = RTCNT;
    __enable_interrupt();

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
__attribute__((__interrupt__(PORT2_VECTOR))) void soft_reset(void){
    uint8_t timer;
    P2IFG &= ~BIT7; //Clear IFG for pin 2.0
    for(timer=0; timer < 12; timer++);
    if(!(P2IN&BIT7)){
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
