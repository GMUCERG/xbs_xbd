/*
 * i2c_comms.h
 *
 *  Created on: Apr 9, 2017
 *      Author: ryan
 */

#ifndef INC_I2C_COMMS_H_
#define INC_I2C_COMMS_H_

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* User can use this section to tailor I2Cx/I2Cx instance used and associated
   resources */
/* Definition for I2Cx clock resources */
// On these STM32 boards, the I2C_ADDRESS is always left shifted by one
#define I2C_ADDRESS_REAL 0x75
#define I2C_ADDRESS (I2C_ADDRESS_REAL << 1)
#ifdef STM32F103xB
#define I2C_SPEEDCLOCK   400000
#elif defined(STM32F091xC)
#define I2C_TIMING      0x00A51314
//#define I2C_TIMING      0x004207A2
#endif
#define I2C_DUTYCYCLE    I2C_DUTYCYCLE_2

#define I2Cx                            I2C1
#define I2Cx_CLK_ENABLE()               __HAL_RCC_I2C1_CLK_ENABLE()
#define I2Cx_SDA_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOB_CLK_ENABLE()
#define I2Cx_SCL_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOB_CLK_ENABLE()

#define I2Cx_FORCE_RESET()              __HAL_RCC_I2C1_FORCE_RESET()
#define I2Cx_RELEASE_RESET()            __HAL_RCC_I2C1_RELEASE_RESET()

/* Definition for I2Cx Pins */
#define I2Cx_SCL_PIN                    GPIO_PIN_8
#define I2Cx_SCL_GPIO_PORT              GPIOB
#define I2Cx_SDA_PIN                    GPIO_PIN_9
#define I2Cx_SDA_GPIO_PORT              GPIOB

// Prototypes
int i2c_init(void);
void i2c_set_rx(void (*i2cSlaveRx_func)(uint8_t
            receiveDataLength, uint8_t* recieveData));
void i2c_set_tx(uint8_t (*i2cSlaveTx_func)(uint8_t
            transmitDataLengthMax, uint8_t* transmitData));
void i2c_handle(void);
void i2c_reset(void);

#endif /* INC_I2C_COMMS_H_ */
