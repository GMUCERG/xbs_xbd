#include <stdio.h>
#include <inttypes.h>

#define F_CPU_DEFAULT (1048576L) 

#define F_CPU F_CPU_DEFAULT

#define RND_UART_RM(x) ((x & 0x3) == 0x3 ? x | 0x4 : x ) >> 2
#define UART_BAUDRATE 115000

int main(void){
    uint32_t value;
    value = RND_UART_RM(((F_CPU<<5)/UART_BAUDRATE)&0x1F)<<1;
    printf("%x\n",value);
}
