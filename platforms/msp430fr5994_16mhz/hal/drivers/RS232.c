#include <msp430.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <inttypes.h>

#include "global.h"
#include "RS232.h"

void usart_init()
{
    P2SEL0 &= ~(BIT0 | BIT1);               // USCI_A0 UART operation
    P2SEL1 |= BIT0 | BIT1;              

    UCA0CTLW0 |= UCSWRST;
    UCA0CTLW0 |= UCSSEL_2;              // CLK = SMCLK(=MCLK)

#if F_CPU >= 16000000
    UCA0BRW = 9;                                        //CLK=16777216 @ baud=115200
    UCA0MCTLW = UCOS16| UCBRF_1| 0xB500;                //CLK=16777216 @ baud=115200
#elif F_CPU >= 8000000
    UCA0BRW = 4;                                        //CLK=8388608 @ baud=115200
    UCA0MCTLW = UCOS16| UCBRF_8| 0xB500;                //CLK=8388608 @ baud=115200
#elif F_CPU >= 4000000
    UCA0BRW = 2;                                        //CLK=4194304 @ baud=115200
    UCA0MCTLW = UCOS16| UCBRF_4| 0x9200;                //CLK=4194304 @ baud=115200
#else
//  UCA0BRW = 8;                                        //CLK=1000000 @ baud=115200
//  UCA0MCTLW = 0xD600;                                 //CLK=1000000 @ baud=115200

    UCA0BRW = 9;                                        //CLK=1048576 @ baud=115200
    UCA0MCTLW = 0x0800;                                 //CLK=1048576 @ baud=115200

//  UCA0BRW = 6;                                        //CLK=1048576 @ baud=9600
//  UCA0MCTLW = UCOS16| UCBRF_13| 0x2200;               //CLK=1048576 @ baud=9600
#endif

    UCA0CTLW0 &= ~UCSWRST;                  // release from reset



/***********************Old Code****************************/

// //     P2SEL |= BIT4|BIT5;                       // P4.7,6 = USCI_A0 RXD/TXD
// //     //P2DIR |= BIT4;                            // Set port dir
// //     //P2DIR &= ~BIT5;                           //set port dir
// //     UCA1CTL1 |= UCSSEL_2|UCSWRST;             // SMCLK
// // //#if F_CPU == F_CPU_DEFAULT
// // //    UCA1BR0 = 0x09;                           // 1MHz 115200
// // //    UCA1BR1 = 0x00;                           // 1MHz 115200
// // //    UCA1MCTL = 0x02;                          // Modulation
// // //#else
// // #define RND_UART_RM(x) (((x) & 0x3) == 0x3 ? (x) | 0x4 : (x)) >> 2 
// //     //dynamically compute baudrate settings
// //     UCA1BR0 = (F_CPU/UART_BAUDRATE)%UINT8_MAX;           // 1MHz 115200
// //     UCA1BR1 = (F_CPU/UART_BAUDRATE)>>8;                  // 1MHz 115200
// //     UCA1MCTL = RND_UART_RM(((F_CPU<<5)/UART_BAUDRATE)&0x1F)<<1;      // Modulation
// // #undef RND_UART_RM
// // //#endif
// //     UCA1CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**

// //     //IE2_bit.UCA1RXIE = 1;       // RX Complete interrupt enabled

//     P2SEL0 &= ~(BIT0 | BIT1);
//     P2SEL1 |= BIT0 | BIT1;                  // USCI_A0 UART operation

//     UCA0CTLW0 |= UCSWRST;
//     UCA0CTLW0 |= UCSSEL_2;              // CLK = SMCLK
//     UCA0BRW = 8;                            // 1000000/115200 = 8.68
//     UCA0MCTLW = 0xD600;                     // 1000000/115200 - INT(1000000/115200)=0.68
//                                             // UCBRSx value = 0xD6 (See UG)
//     UCA0CTLW0 &= ~UCSWRST;                  // release from reset


}

void usart_putc(char data) {
    while(!(UCA0IFG&UCTXIFG));
    UCA0TXBUF = data;
}

char usart_getc(void) {
    while(!(UCA0IFG&UCRXIFG));
    return UCA0RXBUF;                 
}

void usart_putbyte(uint8_t val) {
    char ch;
    uint8_t nibble = (val & 0xf0)>>4;
    if(nibble < 10 && nibble > 0){
        ch='0'+nibble;
    }else if(nibble >=10){
        ch='a'+nibble-10;
    }else{
        ch='0';
    }
    usart_putc(ch);

    nibble = val & 0xf;

    if(nibble < 10 && nibble > 0){
        ch='0'+nibble;
    }else if(nibble >=10){
        ch='a'+nibble-10;
    }else{
        ch='0';
    }
    usart_putc(ch); 
}

void usart_puts(const char *data) {
    int len, count;

    len = strlen(data);
    for (count = 0; count < len; count++) {
        usart_putc(*(data+count));
    }
}

void usart_putint (int usart_num){				  /* Writes integer to the USART		*/
    char usart_ascii_string[6]="      ";		  /* Reservation to the max absvalue (2^16)/2		*/

    //itoa(usart_num, usart_ascii_string, 10);	  /* Convert integer to ascii [not ansi c]		*/
    sprintf(usart_ascii_string, "%d", usart_num); //sprintf <=> itoa
    usart_puts(usart_ascii_string);		          /* Sens ascii pointer to the lcd_printf()		*/
}

void usart_putu32hex(uint32_t num) {
    char s[10];
    //usart_puts( ltoa(num,s,16) );
    sprintf(s,"%lX",num);
    usart_puts(s);
}


