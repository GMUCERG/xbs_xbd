//******************************************************************************
//   MSP430xG41x Demo - USCI_B0 I2C Slave RX multiple bytes from MSP430 Master
//
//   Description: This demo connects two MSP430's via the I2C bus. The master
//   transmits to the slave. This is the slave code. The interrupt driven
//   data receiption is demonstrated using the USCI_B0 RX interrupt.
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

unsigned char *PRxData;                     // Pointer to RX data
unsigned char RXByteCtr;
volatile unsigned char RxBuffer[128];       // Allocate 128 byte of RAM

void main(void)
{
  WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT
  P3SEL |= 0x06;                            // Assign I2C pins to USCI_B0
  UCB0CTL1 |= UCSWRST;                      // Enable SW reset
  UCB0CTL0 = UCMODE_3 + UCSYNC;             // I2C Slave, synchronous mode
  UCB0I2COA = 0x75;                         // Own Address is 048h
  UCB0CTL1 &= ~UCSWRST;                     // Clear SW reset, resume operation
  UCB0I2CIE |= UCSTPIE + UCSTTIE;           // Enable STT and STP interrupt
  IE2 |= UCB0RXIE;                          // Enable RX interrupt

  while (1)
  {
    PRxData = (unsigned char *)RxBuffer;    // Start of RX buffer
    RXByteCtr = 0;                          // Clear RX byte count
    __bis_SR_register(CPUOFF + GIE);        // Enter LPM0 w/ interrupts
                                            // Remain in LPM0 until master
                                            // finishes TX
    __no_operation();                       // Set breakpoint >>here<< and read
  }                                         // read out the RxData buffer
}

//------------------------------------------------------------------------------
// The USCIB0 data ISR is used to move received data from the I2C master
// to the MSP430 memory.
//------------------------------------------------------------------------------
interrupt(USCIAB0TX_VECTOR) USCIAB0TX_ISR(void)
{
  *PRxData++ = UCB0RXBUF;                   // Move RX data to address PRxData
  RXByteCtr++;                              // Increment RX byte count
}

//------------------------------------------------------------------------------
// The USCIB0 state ISR is used to wake up the CPU from LPM0 in order to
// process the received data in the main program. LPM0 is only exit in case
// of a (re-)start or stop condition when actual data was received.
//------------------------------------------------------------------------------
interrupt(USCIAB0RX_VECTOR)  USCIAB0RX_ISR(void)
{
  UCB0STAT &= ~(UCSTPIFG + UCSTTIFG);       // Clear interrupt flags
  if (RXByteCtr)                            // Check RX byte counter
    __bic_SR_register_on_exit(CPUOFF);      // Exit LPM0 if data was
}                                           // received
