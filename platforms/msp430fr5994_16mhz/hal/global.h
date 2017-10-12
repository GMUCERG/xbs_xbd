#ifndef __GLOBAL_H
#define __GLOBAL_H
#include <inttypes.h>


#define F_CPU 16000000L // Speed of crystal on MSP dev board x 32 == approx 1MHz
// #define F_CPU 8000000L
// #define F_CPU 4000000L
// #define F_CPU 1000000L

#define UART_BAUDRATE 115000
#define I2C_ADDR 0x75
#endif
