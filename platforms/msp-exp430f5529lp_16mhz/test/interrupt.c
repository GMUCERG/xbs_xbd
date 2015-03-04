#include <msp430fg4618.h>
#include <legacymsp430.h>
#include <inttypes.h>


void main(void) {
    volatile unsigned int i;
    __disable_interrupt();

    WDTCTL = WDTPW+WDTHOLD;                   // Stop WDT
    //Enable interrupt for soft reset on pin 2.0
    P2SEL &= ~BIT0;
    P2DIR &= ~BIT0;
    P2IES = BIT0;
    P2IE = BIT0;
    P2IFG = 0;
    __enable_interrupt();

    P3SEL |= 0x00;                           // P4.7,6 = USCI_A0 RXD/TXD
    P3DIR |= BIT1|BIT2|BIT3;                          // Set port dir
    P3OUT = BIT1 | BIT3;
    while(1){
        P3OUT ^= BIT1|BIT2|BIT3;
    }
}

interrupt(PORT2_VECTOR)  soft_reset(void){
    P2IFG &= ~BIT0; //Clear IFG for pin 2.0
    //WDTCTL = WDT_MDLY_32;     //(WDTPW+WDTTMSEL+WDTCNTCL+WDTIS1+WDTIS0) Soft reset after 0.064ms at 1MHz
    WDTCTL = 0;     //(WDTPW+WDTTMSEL+WDTCNTCL+WDTIS1+WDTIS0) Soft reset after 0.064ms at 1MHz

    //software reset (this is a soft reset equivalent on msp430 at 32ms 1Mhz smclk)
    //  wdtctl = WDT_MDLY_32; //wdt_enable(WDTO_15MS); //set watch dog WDT_MDLY_32
    while(1);
}

