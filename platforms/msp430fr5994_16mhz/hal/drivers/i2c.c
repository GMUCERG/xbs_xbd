// From msp430-gcc 
//#include <io.h>
//#include <interrupt.h>
//#include <io430xG46x.h>
#include <msp430.h>
#include <stdalign.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <inttypes.h>
//#include "msp430libtypes.h"
//#include <io430xG46x.h>
/*#include <intrinsics.h>
#include <stdint.h>
*/

#include "i2c.h"
#include "XBD_HAL.h"
#include "XBD_debug.h"

//#include "uart2.h"


// Standard I2C bit rates are:

// 100KHz for slow speed

// 400KHz for high speed








// I2C state and address variables


static uint8_t I2cSendDataIndex=0;
static uint8_t I2cSendDataLength=0;
static uint8_t I2cReceiveDataIndex=0;


// Use same buffer for send and receive because tx i2c handler has it's own
// buffer
static alignas(sizeof(uint32_t)) uint8_t I2cBuffer[I2C_SEND_DATA_BUFFER_SIZE];
//static alignas(sizeof(uint32_t)) uint8_t I2cSendData[I2C_SEND_DATA_BUFFER_SIZE];
//static alignas(sizeof(uint32_t)) uint8_t I2cReceiveData[I2C_RECEIVE_DATA_BUFFER_SIZE];
#define I2cSendData I2cBuffer
#define I2cReceiveData I2cBuffer




// function pointer to i2c receive routine
// I2cSlaveReceive is called when this processor
// is addressed as a slave for writing

static void (*i2cSlaveReceive)(uint8_t receiveDataLength, uint8_t* recieveData);

// I2cSlaveTransmit is called when this processor
// is addressed as a slave for reading

static uint8_t (*i2cSlaveTransmit)(uint8_t transmitDataLengthMax, uint8_t* transmitData);



// functions

void i2cInit(void) {

    // P4SEL |= BIT1|BIT2;                       // Assign I2C pins to USCI_B2



    // UCB1CTL1 |= UCSWRST;                      // Enable SW reset
    // UCB1CTL0 &= ~UCMST;
    // UCB1CTL0 = UCMODE_3 | UCSYNC;             // I2C Slave, synchronous mode
    // //UCB1I2COA = 0x75;                         // Own Address is 048h
    // //UCB1CTL0 |= UCA10 | UCSLA10;            // 10 bit addressing 
    // //UCB1CTL1 = UCSSEL_2 | UCSWRST;            // Use SMCLK, keep SW reset
    // UCB2CTL1 &= ~UCSWRST;                     // Clear SW reset, resume operation
    // //UCB1CTL1 &= ~(UCTR|UCSWRST);              // Clear RX and enable I2C state machine


    P1SEL1 |= BIT6 | BIT7;               // Configure I2C pins UCB2
    P1SEL0 &= ~(BIT6 | BIT7);

   
    UCB2CTLW0 |= UCSWRST;                   // Enable SW reset
    UCB2CTLW0 &= ~UCMST;            //slave
    UCB2CTLW0 |= UCMODE_3 | UCSYNC;     //i2c_mode

    UCB0CTLW0 &= ~UCSWRST;      //clear reset

    // clear SlaveReceive and SlaveTransmit handler to null
    i2cSlaveReceive = 0;
    i2cSlaveTransmit = 0;

    //Reset buffer counters
    I2cSendDataIndex=0;
    I2cSendDataLength=0;
    I2cReceiveDataIndex=0;
}





void i2cSetLocalDeviceAddr(uint8_t deviceAddr, uint8_t genCallEn) {
    // set local device address (used in slave mode only)
    UCB2CTLW0 |= UCSWRST;                      // Enable SW reset
    UCB0I2COA0 = ((deviceAddr | (genCallEn?UCGCEN:0)) | UCOAEN);    //own addr. + addr. enable
    UCB2CTLW0 &= ~UCSWRST;                     // Clear SW reset, resume operation
}



void i2cSetSlaveReceiveHandler(void (*i2cSlaveRx_func)(uint8_t receiveDataLength, uint8_t* recieveData)) {
    i2cSlaveReceive = i2cSlaveRx_func;
}



void i2cSetSlaveTransmitHandler(uint8_t (*i2cSlaveTx_func)(uint8_t transmitDataLengthMax, uint8_t* transmitData)) {
    i2cSlaveTransmit = i2cSlaveTx_func;
}





//! I2C (TWI) interrupt service routine



/** dsk: disabled interrupt */

void twi_isr(){ 
//#define DEBUG_I2C
    #define START 0
    #define RX 1
    #define TX 2
    #define TXSTART 3
    #define DONE 4

    static uint8_t state=START;

    switch(state){
        case START:
            if(UCB2IFG&UCSTTIFG){		//start condition?
#ifdef DEBUG_I2C
                XBD_DEBUG("Start\r\n");
#endif
                UCB2IFG &= ~UCSTTIFG;  //yes: clear start flag
                I2cSendDataIndex = 0;	//reset counters
                I2cReceiveDataIndex = 0;
            }
            if(UCB2IFG&UCTXIFG) { 
                state = TX;
            }
            if(UCB2IFG&UCRXIFG) { 
                state = RX;
            }
            break;
        case RX:
            if(UCB2IFG&UCRXIFG) { 
                if(I2cReceiveDataIndex < I2C_RECEIVE_DATA_BUFFER_SIZE) {
                    // receive data byte and return ACK
                    I2cReceiveData[I2cReceiveDataIndex] = UCB2RXBUF;
                    if(!(UCB2IFG&UCSTPIFG)){
                        I2cReceiveDataIndex++;
                    }
#ifdef DEBUG_I2C
                    XBD_DEBUG("RX byte\r\n");
#endif
                } else {
                    XBD_DEBUG("NACK\r\n");
                    // receive data byte and return NACK
                    //I2cReceiveData[I2cReceiveDataIndex] = UCB1RXBUF;
                    UCB2IFG &= ~UCRXIFG;
                    UCB2CTLW0 |= UCTXNACK;
                    //I2cReceiveDataIndex++;
                }
            }
            if(UCB2IFG&UCSTPIFG){
                // i2c receive is complete, call i2cSlaveReceive
#ifdef DEBUG_I2C
                XBD_DEBUG("RX done\r\n");
#endif
                if(i2cSlaveReceive) i2cSlaveReceive(I2cReceiveDataIndex, I2cReceiveData);
                UCB2IFG &= ~UCSTPIFG;
                state=START;
            }
            break;
        case TX:
            if(UCB2IFG&UCTXIFG){       
                if(I2cSendDataIndex==0){
#ifdef DEBUG_I2C
                    XBD_DEBUG("TX start\r\n");
#endif
                    // request data from application

                    if(i2cSlaveTransmit) {I2cSendDataLength = i2cSlaveTransmit(I2C_SEND_DATA_BUFFER_SIZE, I2cSendData);
                    }
                }
#ifdef DEBUG_I2C
                XBD_DEBUG("TX byte\r\n");
#endif
                
                UCB2TXBUF=I2cSendData[I2cSendDataIndex];
                I2cSendDataIndex++;
            }
            if(UCB2IFG&UCSTPIFG){
                UCB2IFG &= ~UCSTPIFG;
                state=START;
            }
            if(UCB2IFG&UCNACKIFG){
                XBD_DEBUG("I2C NAK\r\n");
                state=START;
            };
            break;
    }

}
