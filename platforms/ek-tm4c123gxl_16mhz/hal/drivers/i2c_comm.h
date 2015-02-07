#ifndef _I2C_COMM_H
#define _I2C_COMM_H

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <inc/hw_memmap.h>

//#define I2C_SEND_DATA_BUFFER_SIZE   32
#define I2C_BUFFER_SIZE   160



//! Set the user function which handles receiving (incoming) data as a slave
void i2cSetSlaveReceiveHandler(void (*i2cSlaveRx_func)(uint8_t receiveDataLength, uint8_t* recieveData));

//! Set the user function which handles transmitting (outgoing) data as a slave
void i2cSetSlaveTransmitHandler(uint8_t (*i2cSlaveTx_func)(uint8_t transmitDataLengthMax, uint8_t* transmitData));

/** Sets up i2c tranceiver for XBD comm */
void i2cInit(uint8_t addr);

void i2cHandle(void);

#endif
