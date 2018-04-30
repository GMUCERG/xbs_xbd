/*
 * i2c_comms.c
 *
 *  Created on: Apr 9, 2017
 *      Author: ryan
 *  This file is responsible for handling sending and receiving over I2C.
 */
#include "XBD_HAL.h"
#include "i2c_comms.h"

//#define I2CD 1
#ifdef I2CD
#define I2C_DEBUG(x) printf_xbd(x)
#else
#define I2C_DEBUG(x)
#endif

// Set to 180 per other examples
uint8_t i2c_buf[180] = {0};
uint16_t i2c_size_read = 0;


static void (*i2cSlaveReceive)(uint8_t receiveDataLength, uint8_t* recieveData);

//! I2cSlaveTransmit is called when this processor
// is addressed as a slave for reading
static uint8_t (*i2cSlaveTransmit)(uint8_t transmitDataLengthMax, uint8_t* transmitData);


I2C_HandleTypeDef I2cHandle __attribute__((section("data.i2c")));
void *i2c_ptr = &I2cHandle;
#define I2C_TIMEOUT_FLAG          ((uint32_t)35)     /*!< Timeout 35 ms */
#define I2C_TIMEOUT_ADDR_SLAVE    ((uint32_t)10000)  /*!< Timeout 10 s  */
#define I2C_TIMEOUT_BUSY_FLAG     ((uint32_t)10000)  /*!< Timeout 10 s  */

#ifndef XBD_AF
int i2c_init(void) {
	I2cHandle.Instance             = I2Cx;
	#ifdef STM32F103xB
	I2cHandle.Init.ClockSpeed      = I2C_SPEEDCLOCK;
	I2cHandle.Init.DutyCycle       = I2C_DUTYCYCLE;
	#elif defined(STM32F091xC)
	I2cHandle.Init.Timing          = I2C_TIMING;
	#else
	#error "UNDEFINED I2C INIT"	
	#endif
	I2cHandle.Init.OwnAddress1     = I2C_ADDRESS;
	I2cHandle.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
	I2cHandle.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	I2cHandle.Init.OwnAddress2     = I2C_ADDRESS;
	I2cHandle.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	I2cHandle.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;

	I2cHandle.Instance             = I2Cx;
	

	I2cHandle.State = HAL_I2C_STATE_RESET;
	if(HAL_I2C_Init(&I2cHandle) != HAL_OK)
	{
		printf_xbd("Failed to initialize i2c\n");
		/* Initialization Error */
		return 1;
	}
	#if defined(STM32F091xC)
	HAL_I2CEx_ConfigAnalogFilter(&I2cHandle,I2C_ANALOGFILTER_ENABLE);
	#elif defined(STM32F103xB)
	#endif

	BSP_LED_Init(LED2);

	return 0;
}

#else
int i2c_init(void) {
	// This was already setup during boot, the same structure should be in memory at the same point
	i2c_ptr = (void *)0x20000434;
	return 0;
}
#endif

void i2c_set_rx(void (*i2cSlaveRx_func)(uint8_t
            receiveDataLength, uint8_t* recieveData)) {
	i2cSlaveReceive = i2cSlaveRx_func;
}


void i2c_set_tx(uint8_t (*i2cSlaveTx_func)(uint8_t
            transmitDataLengthMax, uint8_t* transmitData)) {
	i2cSlaveTransmit = i2cSlaveTx_func;
}


void Error_Handler(void) {
	 /* Error if LED2 is slowly blinking (1 sec. period) */
	  while(1)
	  {
	    BSP_LED_Toggle(LED2);
	    HAL_Delay(1000);
	  }
}

static HAL_StatusTypeDef I2C_WaitOnFlagUntilTimeout_xbd(I2C_HandleTypeDef *hi2c, uint32_t Flag, FlagStatus Status, uint32_t Timeout)
{
  uint32_t tickstart = 0;

  /* Get tick */
  tickstart = HAL_GetTick();

  /* Wait until flag is set */
  if(Status == RESET)
  {
    while(__HAL_I2C_GET_FLAG(hi2c, Flag) == RESET)
    {
      /* Check for the Timeout */
      if(Timeout != HAL_MAX_DELAY)
      {
        if((Timeout == 0)||((HAL_GetTick() - tickstart ) > Timeout))
        {
          hi2c->State= HAL_I2C_STATE_READY;

          /* Process Unlocked */
          __HAL_UNLOCK(hi2c);

          return HAL_TIMEOUT;
        }
      }
    }
  }
  else
  {
    while(__HAL_I2C_GET_FLAG(hi2c, Flag) != RESET)
    {
      /* Check for the Timeout */
      if(Timeout != HAL_MAX_DELAY)
      {
        if((Timeout == 0)||((HAL_GetTick() - tickstart ) > Timeout))
        {
          hi2c->State= HAL_I2C_STATE_READY;

          /* Process Unlocked */
          __HAL_UNLOCK(hi2c);

          return HAL_TIMEOUT;
        }
      }
    }
  }
  return HAL_OK;
}

#if defined(STM32F103xB)
#define TRANSMIT_FLAG I2C_FLAG_TXE
#define END_TRANSMIT_FLAG I2C_FLAG_AF				
void address_ack(int en) {
	I2C_HandleTypeDef *hi2c = &I2cHandle;
	if (en) {
		SET_BIT(hi2c->Instance->CR1, I2C_CR1_ACK);
	} else {

	}
	
}
#elif defined(STM32F091xC)

#endif

#if defined(STM32F091xC)
void i2c_handle(void) {
	I2C_HandleTypeDef *hi2c = i2c_ptr;
	uint8_t *pData = (uint8_t *)i2c_buf;
	uint32_t Timeout = 100000;
	i2c_size_read = 0;
	I2C_DEBUG("About to handle i2c comms\n");

	if(hi2c->State == HAL_I2C_STATE_READY)
	  {
	
		hi2c->Mode = HAL_I2C_MODE_SLAVE;
		hi2c->ErrorCode = HAL_I2C_ERROR_NONE;

		/* Enable Address Acknowledge */
		hi2c->Instance->CR2 &= ~I2C_CR2_NACK;


		I2C_DEBUG("waiting for i2c comms\n");

		/* Wait until ADDR flag is set */
		if(I2C_WaitOnFlagUntilTimeout_xbd(hi2c, I2C_FLAG_ADDR, RESET, Timeout) != HAL_OK)
		{
			return;
		}
		I2C_DEBUG("received addr flag\n");

		/* Clear ADDR flag */
		
		__HAL_I2C_CLEAR_FLAG(hi2c,I2C_FLAG_ADDR);

		/*#if defined(STM32F091xC)
		if (__HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_TXIS) == SET) {
			I2C_DEBUG("TX FLAG SET\n");
			if (__HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_DIR) == SET) {
				I2C_DEBUG("DIR FLAG SET\n");
			}
		}

		if (__HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_RXNE) == SET) {
			I2C_DEBUG("RX FLAG SET\n");

			if (__HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_DIR) == SET) {
				I2C_DEBUG("DIR FLAG SET\n");
			}
		}
		
		#endif	*/
		

		while (!((__HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_TXIS) == SET && __HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_DIR) == SET) ||
		  (__HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_DIR) == RESET && __HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_RXNE) == SET)));
		if (__HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_DIR) == RESET) {
			I2C_DEBUG("I2C Master is attempting to send\n");
			uint8_t rcv = 1;
			while (rcv == 1) {
				while(__HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_RXNE) == RESET)
				{

					/* Check if a STOPF is detected */
					if(__HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_STOPF) == SET)
					{
						/* Clear STOP Flag */
						__HAL_I2C_CLEAR_FLAG(hi2c, I2C_FLAG_STOPF);
						 hi2c->Instance->CR2 |= I2C_CR2_NACK;
						#if defined(STM32F091xC)
						if(I2C_WaitOnFlagUntilTimeout_xbd(hi2c, I2C_FLAG_BUSY, SET, Timeout) != HAL_OK)
						{
							/* Disable Address Acknowledge */
							hi2c->Instance->CR2 |= I2C_CR2_NACK;
							return;
						}
						#endif
						hi2c->ErrorCode = HAL_I2C_ERROR_NONE;
						hi2c->State= HAL_I2C_STATE_READY;

						I2C_DEBUG("Received message from I2C Master\n");
						i2cSlaveReceive(i2c_size_read, i2c_buf);
						rcv = 0;
						
						
						return;
					}
				}


				(*pData++) = hi2c->Instance->RXDR;
				i2c_size_read++;
			}


		} else if (__HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_DIR) == SET) {

			I2C_DEBUG("I2C Master is attempting to receive\n");
			volatile unsigned int sent = 0;
			i2cSlaveTransmit(160, i2c_buf);
			uint8_t tx = 1;
			while (tx == 1) {
				while(__HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_TXIS) == RESET)
				{
					if(__HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_AF) == SET ||
					  __HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_STOPF) == SET) {
						I2C_DEBUG("Stop set\n");
						//while (__HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_TXIS) == RESET)
						//{
						//}
						//if (__HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_TXIS) == SET) {
						//	asm volatile ("bkpt");
						//}
						/* Clear AF flag */
						//hi2c->Instance->TXDR = (*pData++);
						//sent++;
						if (__HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_AF) == SET) {
							__HAL_I2C_CLEAR_FLAG(hi2c, I2C_FLAG_AF);
						}
						while (__HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_STOPF) == RESET);
						/* Clear STOP Flag */
						__HAL_I2C_CLEAR_FLAG(hi2c, I2C_FLAG_STOPF);
						 hi2c->Instance->CR2 |= I2C_CR2_NACK;

						if(I2C_WaitOnFlagUntilTimeout_xbd(hi2c, I2C_FLAG_BUSY, SET, Timeout) != HAL_OK)
						{
							/* Disable Address Acknowledge */
							hi2c->Instance->CR2 |= I2C_CR2_NACK;
							return;
						}

						hi2c->ErrorCode = HAL_I2C_ERROR_NONE;
						hi2c->State= HAL_I2C_STATE_READY;

						I2C_DEBUG("Transmitted message to I2C Master\n");
						if (sent > 0) {
							print_int_xbd(sent);
						}
						// Not sure why these steps are required at the moment
						// Some HAL functions do this after a transfer, and if
						// I don't do it, there is an off by one transfer (each
						// subsequent transfer is one byte less than expected)
						/* Clear Configuration Register 2 */
						I2C_RESET_CR2(hi2c);

						/* Flush TX register */
						if(__HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_TXIS) != RESET)
						{
							hi2c->Instance->TXDR = 0x00U;
						}

						/* Flush TX register if not empty */
						if(__HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_TXE) == RESET)
						{
							__HAL_I2C_CLEAR_FLAG(hi2c, I2C_FLAG_TXE);
						}
						tx = 0;
						return;
					}
				}
				
				hi2c->Instance->TXDR = (*pData++);
				sent++;
			}
		}
	}
}
#endif

#if defined(STM32F103xB)
void i2c_reset(void) {
	HAL_I2C_DeInit(&I2cHandle);
}

void i2c_handle(void) {

	

	I2C_HandleTypeDef *hi2c = i2c_ptr;
	uint8_t *pData = (uint8_t *)i2c_buf;
	uint32_t Timeout = 100000;
	i2c_size_read = 0;
	I2C_DEBUG("About to handle i2c comms\n");


	if(hi2c->State == HAL_I2C_STATE_READY)
	  {



	    /* Disable Pos */
	    CLEAR_BIT(hi2c->Instance->CR1, I2C_CR1_POS);

	    hi2c->State = HAL_I2C_STATE_BUSY_RX;
	    hi2c->Mode = HAL_I2C_MODE_SLAVE;
	    hi2c->ErrorCode = HAL_I2C_ERROR_NONE;

	    /* Enable Address Acknowledge */
	    SET_BIT(hi2c->Instance->CR1, I2C_CR1_ACK);

	    /* Wait until ADDR flag is set */
	   if(I2C_WaitOnFlagUntilTimeout_xbd(hi2c, I2C_FLAG_ADDR, RESET, Timeout) != HAL_OK)
	   {
		 return;
	   }
	   pData = (uint8_t *)i2c_buf;
	   I2C_DEBUG("received addr flag\n");

	   /* Clear ADDR flag */
	   __HAL_I2C_CLEAR_ADDRFLAG(hi2c);

	  while (__HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_TXE) == RESET && __HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_RXNE) == RESET);

	   if (__HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_RXNE) == SET) {
		   I2C_DEBUG("Master is attempting to send\n");
		   uint8_t rcv = 1;
		   while (rcv == 1) {
			   while(__HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_RXNE) == RESET)
				{

				  /* Check if a STOPF is detected */
				  if(__HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_STOPF) == SET)
				  {
					/* Clear STOP Flag */
					__HAL_I2C_CLEAR_STOPFLAG(hi2c);

					//CLEAR_BIT(hi2c->Instance->CR1, I2C_CR1_ACK);
					hi2c->ErrorCode = HAL_I2C_ERROR_NONE;
					hi2c->State= HAL_I2C_STATE_READY;

					/* Process Unlocked */

					I2C_DEBUG("Received message from Master\n");
					// Make sure to clear all reads
					(*pData++) = hi2c->Instance->DR;
					I2C_DEBUG(i2c_buf);
					i2cSlaveReceive(i2c_size_read, i2c_buf);
					rcv = 0;
					return;
				  }
				}
			   /* Read data from DR */
			(*pData++) = hi2c->Instance->DR;
			i2c_size_read++;

			if((__HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_BTF) == SET))
			{
			/* Read data from DR */
			(*pData++) = hi2c->Instance->DR;
				i2c_size_read++;
			}

	}

	   } else if (__HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_TXE) == SET) {
		   I2C_DEBUG("Master is attempting to receive\n");
		   uint8_t resp_size = i2cSlaveTransmit(160, i2c_buf);
		   uint8_t tx = 1;
		   volatile unsigned int sent = 0;
		   while (tx == 1) {
			   while(__HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_TXE) == RESET)
				{
				   if(__HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_AF) == SET)
				    {
					   /* Clear AF flag */
					 __HAL_I2C_CLEAR_FLAG(hi2c, I2C_FLAG_AF);

					hi2c->ErrorCode = HAL_I2C_ERROR_NONE;
					hi2c->State= HAL_I2C_STATE_READY;

					if (sent > 0) {
						print_int_xbd(sent);
					}

					I2C_DEBUG("Transmitted message to Master\n");
					//CLEAR_BIT(hi2c->Instance->CR1, I2C_CR1_ACK);
					tx = 0;
					return;
				  } else if (__HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_ADDR) == SET) {
					// Got stuck in loop
					printf_xbd("Error, stuck in loop\n");
					HAL_I2C_DeInit(&I2cHandle);
					HAL_I2C_Init(&I2cHandle);
					return;
				  }
				}

			   /* Write data to DR */
				hi2c->Instance->DR = (*pData++);
				sent++;

				 if((__HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_BTF) == SET))
				 {
				   /* Write data to DR */
				   hi2c->Instance->DR = (*pData++);
				 }
		   }
	   }

	  }
}
#endif

