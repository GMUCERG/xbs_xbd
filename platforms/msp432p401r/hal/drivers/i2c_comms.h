#ifndef _I2C_COMMS_H_
#define _I2C_COMMS_H_

#define SLAVE_ADDRESS       0x75

void i2c_set_rx(void (*i2cSlaveRx_func)(uint8_t receiveDataLength, 
  uint8_t* recieveData));

void i2c_set_tx(uint8_t (*i2cSlaveTx_func)(uint8_t transmitDataLengthMax, 
  uint8_t* transmitData));

void i2c_init(void);

void EUSCIB1_IRQHandler(void);

void i2c_rx(void);


#endif
