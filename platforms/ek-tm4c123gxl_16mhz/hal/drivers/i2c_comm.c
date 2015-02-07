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
#if 0
    //MAP_GPIOPinTypeI2CSCL(GPIO_PORTA_BASE, GPIO_PIN_6);
    //MAP_GPIOPinTypeI2C(GPIO_PORTA_BASE, GPIO_PIN_7);
    GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, GPIO_PIN_6);
    GPIOPinTypeGPIOOutputOD(GPIO_PORTA_BASE, GPIO_PIN_7);

    MAP_GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_7, GPIO_PIN_7);
    MAP_GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_6, GPIO_PIN_6);
    while(1){
        MAP_GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_7|GPIO_PIN_6, GPIO_PIN_7);
        MAP_GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_7|GPIO_PIN_6, GPIO_PIN_6);
    }

#else
    // Configure I2C pins
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C1);
    MAP_SysCtlPeripheralReset(SYSCTL_PERIPH_I2C1);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    MAP_GPIOPinConfigure(GPIO_PA6_I2C1SCL);
    MAP_GPIOPinConfigure(GPIO_PA7_I2C1SDA);
    MAP_GPIOPinTypeI2CSCL(GPIO_PORTA_BASE, GPIO_PIN_6);
    MAP_GPIOPinTypeI2C(GPIO_PORTA_BASE, GPIO_PIN_7);
    MAP_I2CSlaveInit(I2C1_BASE, addr);
    MAP_I2CSlaveEnable(I2C1_BASE);
#endif
}

typedef enum {
    STATE_START, 
    STATE_RX, 
    STATE_TX
} state_t;
void i2cHandle(void){
    state_t state = STATE_START;
    uint32_t status;
    bool invalid;
    size_t rx_idx;
    size_t tx_idx;

    while(true){
        status = I2CSlaveStatus(I2C1_BASE);

        switch(state){
            case STATE_START:
                if(I2CSlaveIntStatusEx(I2C1_BASE, false) & I2C_SLAVE_INT_START){
                    rx_idx = 0;
                    tx_idx = 0;
                    invalid = false;
                    if(status == I2C_SLAVE_ACT_RREQ_FBR){
                        I2CSlaveIntClearEx(I2C1_BASE, I2C_SLAVE_INT_START);
                        state = STATE_RX;
                    }else if(status == I2C_SLAVE_ACT_TREQ){
                        I2CSlaveIntClearEx(I2C1_BASE, I2C_SLAVE_INT_START);
                        state = STATE_TX;
                        break;
                    }
                }
                break;
            case STATE_RX:
                if(status == I2C_SLAVE_ACT_RREQ ||
                        status == I2C_SLAVE_ACT_RREQ_FBR){
                    // If invalid, keep reading but discard
                    if(rx_idx >= I2C_RECEIVE_DATA_BUFFER_SIZE){
                        rx_idx = 0;
                        invalid = true;
                    }
                    I2cReceiveData[rx_idx++] = MAP_I2CSlaveDataGet(I2C1_BASE);
                }
                if(I2CSlaveIntStatusEx(I2C1_BASE, false) & I2C_SLAVE_INT_STOP){
                    I2CSlaveIntClearEx(I2C1_BASE, I2C_SLAVE_INT_STOP);
                    state = STATE_START;
                    if(!invalid){
                        i2cSlaveReceive(rx_idx, I2cReceiveData);
                    }
                }
                break;
            case STATE_TX:
                if(status == I2C_SLAVE_ACT_TREQ){
                    // If invalid, keep reading but discard
                    if(tx_idx >= I2C_SEND_DATA_BUFFER_SIZE){
                        tx_idx = 0;
                        invalid = true;
                    }
                    MAP_I2CSlaveDataPut(I2C1_BASE, I2cSendData[tx_idx++]);
                }
                if(I2CSlaveIntStatusEx(I2C1_BASE, false) & I2C_SLAVE_INT_STOP){
                    I2CSlaveIntClearEx(I2C1_BASE, I2C_SLAVE_INT_STOP);
                    state = STATE_START;
                    if(!invalid){
                        i2cSlaveTransmit(I2C_SEND_DATA_BUFFER_SIZE, I2cSendData);
                    }
                }
                break;
        }
#if 0
/*{{{*/
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

        last_status = status;/*}}}*/
#endif
    }
}



