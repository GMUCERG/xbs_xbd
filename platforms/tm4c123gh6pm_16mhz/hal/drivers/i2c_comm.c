#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

#include <driverlib/rom.h>
#include <driverlib/rom_map.h>

#include <inc/hw_ints.h>
#include <inc/hw_memmap.h>
#include <driverlib/gpio.h>
#include <driverlib/pin_map.h>
#include <driverlib/sysctl.h>
#include <driverlib/i2c.h>

#include "i2c_comm.h"


static uint8_t I2cSendData[I2C_SEND_DATA_BUFFER_SIZE];
static uint8_t I2cReceiveData[I2C_RECEIVE_DATA_BUFFER_SIZE];

// function pointer to i2c receive routine
//! I2cSlaveReceive is called when this processor
// is addressed as a slave for writing

static void (*i2cSlaveReceive)(uint8_t receiveDataLength, uint8_t* recieveData);

//! I2cSlaveTransmit is called when this processor
// is addressed as a slave for reading
static uint8_t (*i2cSlaveTransmit)(uint8_t transmitDataLengthMax, uint8_t* transmitData);

void i2cSetSlaveReceiveHandler(void (*i2cSlaveRx_func)(uint8_t
            receiveDataLength, uint8_t* recieveData)) { i2cSlaveReceive =
    i2cSlaveRx_func; }


void i2cSetSlaveTransmitHandler(uint8_t (*i2cSlaveTx_func)(uint8_t
            transmitDataLengthMax, uint8_t* transmitData)) { i2cSlaveTransmit =
    i2cSlaveTx_func;
}

void i2cInit(uint8_t addr){
    // Configure I2C pins
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C1);
    MAP_SysCtlPeripheralReset(SYSCTL_PERIPH_I2C1);
    MAP_GPIOPinConfigure(GPIO_PA6_I2C1SCL);
    MAP_GPIOPinConfigure(GPIO_PA7_I2C1SDA);
    MAP_GPIOPinTypeI2CSCL(GPIO_PORTA_BASE, GPIO_PIN_6);
    MAP_GPIOPinTypeI2C(GPIO_PORTA_BASE, GPIO_PIN_7);
    MAP_I2CSlaveInit(I2C1_BASE, addr);
    MAP_I2CSlaveEnable(I2C1_BASE);
}

void i2cHandle(void){
    uint32_t status;
    uint32_t last_status;
    bool invalid = false;
    size_t rx_idx = 0;
    size_t tx_idx = 0;

    last_status = I2C_SLAVE_ACT_NONE;
    while(true){
        // Mask out FBR bit, since we don't care and treat it as RREQ
        status = I2CSlaveStatus(I2C1_BASE) & ~(I2C_SLAVE_ACT_RREQ_FBR & (~I2C_SLAVE_ACT_RREQ));

        // Protect against buffer overflows
        if(rx_idx >= I2C_RECEIVE_DATA_BUFFER_SIZE){
            rx_idx = 0;
            invalid = true;
        }
        if(tx_idx >= I2C_SEND_DATA_BUFFER_SIZE){
            tx_idx = 0;
            invalid = true;
        }

        // If state transition, perform appropriate actions
        if(status != last_status){
            if(last_status == I2C_SLAVE_ACT_RREQ){
                i2cSlaveReceive(rx_idx, I2cReceiveData);
                rx_idx = 0;
            }
            if(status == I2C_SLAVE_ACT_TREQ){
                tx_idx = 0;
                i2cSlaveTransmit(I2C_SEND_DATA_BUFFER_SIZE, I2cSendData);
            }
        }

        switch(status){
            case I2C_SLAVE_ACT_RREQ:
                I2cReceiveData[rx_idx++] = MAP_I2CSlaveDataGet(I2C1_BASE);
                break;
            case I2C_SLAVE_ACT_TREQ:
                MAP_I2CSlaveDataPut(I2C1_BASE, I2cSendData[tx_idx++]);
                break;
            default:
                break;
        }

        last_status = status;
    }
}



