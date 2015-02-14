//******************************************************************************
//   MSP430xG461x Demo - USCI_B0 I2C Slave TX multiple bytes to MSP430 Master
//
//   Description: This demo connects two MSP430's via the I2C bus. The slave
//   transmits to the master. This is the slave code. The interrupt driven
//   data transmission is demonstrated using the USCI_B0 TX interrupt.
//
//                                 /|\  /|\
//                MSP430xG461x     10k  10k    MSP430xG461x
//                    slave         |    |        master
//              -----------------   |    |  -----------------
//            -|XIN  P3.1/UCB0SDA|<-|---+->|P3.1/UCB0SDA  XIN|-
//       32kHz |                 |  |      |                 | 32kHz
//            -|XOUT             |  |      |             XOUT|-
//             |     P3.2/UCB0SCL|<-+----->|P3.2/UCB0SCL     |
//             |                 |         |                 |
//
//   Andreas Dannenberg/ M. Mitchell
//   Texas Instruments Inc.
//   October 2006
//   Built with CCE Version: 3.2.0 and IAR Embedded Workbench Version: 3.41A
//******************************************************************************
#include <msp430fg4618.h>
#include <legacymsp430.h>

unsigned char *PTxData;                     // Pointer to TX data
volatile unsigned char TXByteCtr;
const unsigned char TxData[] =              // Table of data to transmit
{
  0x11,
  0x22,
  0x33,
  0x44,
  0x55
};

void main(void)
{
  WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT
  P2SEL &= BIT2;
  P2DIR |= BIT2;

  P3SEL |= 0x06;                            // Assign I2C pins to USCI_B0
  UCB0CTL1 |= UCSWRST;                      // Enable SW reset
  UCB0CTL0 = UCMODE_3 + UCSYNC;             // I2C Slave, synchronous mode
  UCB0I2COA = 0x00;                         // Own Address is 048h
  UCB0CTL1 &= ~UCSWRST;                     // Clear SW reset, resume operation
  P2OUT &= ~BIT2;
//  while(1){
//      if(UCB0STAT & UCSTTIFG){
//          UCB0STAT &= ~UCSTTIFG;
//          P2OUT |= BIT2;
//      }
//  }
          

UCB0I2CIE |= UCSTPIE + UCSTTIE;           // Enable STT and STP interrupt
IE2 |= UCB0TXIE;                          // Enable TX interrupt

while (1)
{
  PTxData = (unsigned char *)TxData;      // Start of TX buffer
  TXByteCtr = 0;                          // Clear TX byte count
  __bis_SR_register(CPUOFF + GIE);        // Enter LPM0 w/ interrupts
                                          // Remain in LPM0 until master
                                          // finishes RX
  __no_operation();                       // Set breakpoint >>here<< and
}                                         // read out the TXByteCtr counter
}

//------------------------------------------------------------------------------
// The USCIB0 data ISR is used to move data from MSP430 memory to the
// I2C master. PTxData points to the next byte to be transmitted, and TXByteCtr
// keeps track of the number of bytes transmitted.
//------------------------------------------------------------------------------
interrupt(USCIAB0TX_VECTOR) USCIAB0TX_ISR(void)
{
  UCB0TXBUF = *PTxData++;                   // Transmit data at address PTxData
  TXByteCtr++;                              // Increment TX byte counter
}

//------------------------------------------------------------------------------
// The USCIB0 state ISR is used to wake up the CPU from LPM0 in order to do
// processing in the main program after data has been transmitted. LPM0 is
// only exit in case of a (re-)start or stop condition when actual data
// was transmitted.
//------------------------------------------------------------------------------
interrupt(USCIAB0RX_VECTOR)  USCIAB0RX_ISR(void)
{
  UCB0STAT &= ~(UCSTPIFG + UCSTTIFG);       // Clear interrupt flags
  if (TXByteCtr)                            // Check TX byte counter
    __bic_SR_register_on_exit(CPUOFF);      // Exit LPM0 if data was
}                                           // transmitted
