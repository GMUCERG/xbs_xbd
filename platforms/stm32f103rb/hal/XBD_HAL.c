
/*
 * XBD_HAL.c
 *
 *  Created on: Apr 9, 2017
 *      Author: ryan
 */


#include "XBD_HAL.h"
#include <XBD_FRW.h>
#include "i2c_comms.h"

#define TF_GPIO_PORT              GPIOA
#define TF_PIN                    GPIO_PIN_6

#define STACK_CANARY (inv_sc?0x3A:0xC5)

extern uint32_t _end;

char * print_ptr;
unsigned int integer_print;
// Set optimization to 0 so this doesn't get removed by compiler
int __attribute__((optimize("O0")))
printf_gdb(char *str)
{
	print_ptr = str;
	return 0;
}

int __attribute__((optimize("O0")))
print_int_gdb(unsigned int print_int)
{
	integer_print = print_int;
	return 0;
}

void __attribute__((optimize("O0")))
printf_xbd(char *str)
{
	print_ptr = str;
	printf_gdb(str);
}

int __attribute__((optimize("O0")))
print_int_xbd(unsigned int print_int)
{
	integer_print = print_int;
	print_int_gdb(print_int);
	return 0;
}

void memcpy(void *dst, void *src, uint32_t size)
{
	unsigned int i = 0;
	for (i = 0; i < size; i++) {
		((char *)dst)[i] = ((char *)src)[i];
	}
}

void strcpy(char *dst, const char *src)
{
	unsigned int i = 0;
	for (i = 0; ((char *)src)[i] != 0; i++) {
		((char *)dst)[i] = ((char *)src)[i];
	}
	((char *)dst)[i] = 0;
}
#if defined(STM32F103xB)
void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef clkinitstruct = {0};
  RCC_OscInitTypeDef oscinitstruct = {0};

  /* Configure PLL ------------------------------------------------------*/
  /* PLL configuration: PLLCLK = (HSI / 2) * PLLMUL = (8 / 2) * 4 = 16 MHz */
  /* PREDIV1 configuration: PREDIV1CLK = PLLCLK / HSEPredivValue = 16 / 1 = 16 MHz */
  /* Enable HSI and activate PLL with HSi_DIV2 as source */
  oscinitstruct.OscillatorType  = RCC_OSCILLATORTYPE_HSI;
  oscinitstruct.HSEState        = RCC_HSE_OFF;
  oscinitstruct.LSEState        = RCC_LSE_OFF;
  oscinitstruct.HSIState        = RCC_HSI_ON;
  oscinitstruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  oscinitstruct.HSEPredivValue    = RCC_HSE_PREDIV_DIV1;
  oscinitstruct.PLL.PLLState    = RCC_PLL_ON;
  oscinitstruct.PLL.PLLSource   = RCC_PLLSOURCE_HSI_DIV2;
  oscinitstruct.PLL.PLLMUL      = RCC_PLL_MUL4;

  if (HAL_RCC_OscConfig(&oscinitstruct)!= HAL_OK)
  {
    /* Initialization Error */
	return;
  }

  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
     clocks dividers */
  clkinitstruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  clkinitstruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  clkinitstruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  clkinitstruct.APB2CLKDivider = RCC_HCLK_DIV1;
  clkinitstruct.APB1CLKDivider = RCC_HCLK_DIV2;
  if (HAL_RCC_ClockConfig(&clkinitstruct, FLASH_LATENCY_2)!= HAL_OK)
  {
    /* Initialization Error */
	return;
  }
}
#elif defined(STM32F091xC)
void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;
  
  /* Select HSI48 Oscillator as PLL source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI48;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct)!= HAL_OK)
  {
    /* Initialization Error */
    while(1); 
  }

  /* Select PLL as system clock source and configure the HCLK and PCLK1 clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1)!= HAL_OK)
  {
    /* Initialization Error */
    while(1); 
  }
}
#endif

void XBD_init() {
	RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
        volatile unsigned int test = AFIO->MAPR;
        SET_BIT(AFIO->MAPR, AFIO_MAPR_I2C1_REMAP);
        test = AFIO->MAPR;
	volatile unsigned int *tet = &(AFIO->MAPR);
	AFIO->MAPR |= 0x2;
	

	// Call init using STM32Cube HAL library
	HAL_Init();

	/* Configure the system clock to 64 MHz */
	SystemClock_Config();


	// Prepare i2c communications
	if (i2c_init()) {
		printf_xbd("Failed to initialize i2c comms !\n");
	}

	i2c_set_rx(FRW_msgRecHand);
	i2c_set_tx(FRW_msgTraHand);

	// Initialize GPIO pin for execution signal
	static GPIO_InitTypeDef  GPIO_InitStruct;
	GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull  = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

	GPIO_InitStruct.Pin = TF_PIN;
	HAL_GPIO_Init(TF_GPIO_PORT, &GPIO_InitStruct);
	HAL_GPIO_WritePin(TF_GPIO_PORT, TF_PIN, GPIO_PIN_SET);

	HAL_SuspendTick();
}

void XBD_serveCommunication(void) {
	// Call underlying receive/send
	i2c_handle();
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *I2cHandle)
{
	//Error callback from HAL, STM32Cube
	printf_xbd("Error in I2C!\n");
}

void XBD_loadStringFromConstDataArea( char *dst, const char *src  ) {
  /* copy a zero terminated string from src (CONSTDATAAREA) to dst
  e.g. strcpy if CONSTDATAREA empty, strcpy_P for PROGMEM
  */
  strcpy(dst,src);
}

void XBD_readPage( uint32_t pageStartAddress, uint8_t * buf ) {
  /* read PAGESIZE bytes from the binary buffer, index pageStartAddress
  to buf */
  memcpy(buf,(uint8_t *)pageStartAddress,PAGESIZE);
}

void XBD_programPage(uint32_t pageStartAddress, uint8_t *buf) {
	static FLASH_EraseInitTypeDef EraseInitStruct;
	uint32_t PAGEError = 0, address = pageStartAddress, i = 0;

	// Unlock flash for writing
	HAL_FLASH_Unlock();
	EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
	EraseInitStruct.PageAddress = pageStartAddress;
	EraseInitStruct.NbPages     = 1;


	// First erase the page (i.e. knock all bits to 1)
	if (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK) {
		printf_xbd("Failed to erase requested page\n");
		return;
	}

	//printf_xbd("Erased page successfully\n");

	unsigned int tmp_address = address;

	// Next program each word of the page using the supplied buffer
	for (i = 0; i < PAGESIZE; i += 4) {
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, *(uint32_t *)(&buf[i])) != HAL_OK) {
			printf_xbd("Failed to program requested page, retrying\n");
			HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError);
			address = tmp_address;
			i = -4;
			
		} else {
			//printf_xbd("programmed word\n");
			address += 4;
		}
	}

	// Lock flash for writing
	HAL_FLASH_Lock();

}

// Need to be defined for compilation, however, aren't needed on platform
void XBD_stopWatchDog(void)
{
	
}

void XBD_startWatchDog(uint32_t seconds)
{

}

inline void XBD_sendExecutionStartSignal() {
	HAL_GPIO_WritePin(TF_GPIO_PORT, TF_PIN, GPIO_PIN_RESET);
}

inline void XBD_sendExecutionCompleteSignal() {
	HAL_GPIO_WritePin(TF_GPIO_PORT, TF_PIN, GPIO_PIN_SET);
}

void soft_reset(void) {
	HAL_NVIC_SystemReset();
}

void XBD_switchToBootLoader(void) {
	HAL_NVIC_SystemReset();
}

void XBD_switchToApplication() {
	asm("mov pc,%[addr]"::[addr] "r" (FLASH_ADDR_MIN));
}

uint32_t XBD_busyLoopWithTiming(uint32_t approxCycles) {
	volatile uint32_t exactCycles = 0;

	HAL_SuspendTick();


	uint32_t ctrl = SysTick->CTRL;
	SysTick->LOAD = approxCycles + 1000;
	SysTick->VAL = 0;
	SysTick->CTRL = 5;
	XBD_sendExecutionStartSignal();
	exactCycles=SysTick->VAL;
	while ((exactCycles=SysTick->VAL) > 1000);

	XBD_sendExecutionCompleteSignal();

	exactCycles = approxCycles + 1000 - SysTick->VAL;

	/*Configure the SysTick to have interrupt in 1ms time basis*/
	//HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);
	SysTick->CTRL = ctrl;
	HAL_InitTick(TICK_INT_PRIORITY);
	HAL_ResumeTick();
	return exactCycles;
}

volatile uint8_t *p_stack = 0;
uint8_t *p = 0;
uint8_t inv_sc=0;


__attribute__ ( ( noinline ) )
void getSP(volatile uint8_t **var_SP)
{
  volatile uint8_t beacon;
  *var_SP=&beacon;
  return;
}

uint32_t XBD_countStack(void) {
/* return stack measurement result */
    p = (uint8_t *)&_end;
    register uint32_t c = 0;

    while(*p == STACK_CANARY && p <= p_stack)
    {
        ++p;
        ++c;
    }

    return (uint32_t)c;
}

void XBD_paintStack(void) {
    //register void * __stackptr asm("sp");   ///<Access to the stack pointer
/* initialise stack measurement */
	p = (uint8_t *)&_end; 
	inv_sc=!inv_sc;
	getSP(&p_stack);
    //p_stack = __stackptr;

    
	while(p < p_stack)
        {
        *p = STACK_CANARY;
        ++p;
        } 
}
