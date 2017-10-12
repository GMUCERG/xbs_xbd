/*------------------New Code-------------------------------*/

#include <msp430.h>
#include "global.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
/* From http://mspgcc.sourceforge.net/manual/c1408.html */

static void __inline__ delay_cycles(unsigned long n)
{

   	RTCCTL0_H = RTCKEY_H;                      //unlock RTC_C for writes
   	RTCCTL13 = RTCHOLD | RTCSSEL0 | RTCTEV1|RTCTEV0;
#define RTCNT (*((volatile uint32_t*) &RTCCNT1))
   	RTCNT = 0;                               //reset ctr
   	RTCCTL13 &= ~RTCHOLD;
//   	while(RTCNT < n);			//Why RTCNT? RTCNT uses ACLK(/=MCLK)...
   	RTCCTL13 |= RTCHOLD;
   	RTCCTL0_H = 0;      					//lock RTC_C again
}
#pragma GCC diagnostic pop

static void enableXtal()
{
	/* LFXT can take up to 1000ms to start.
	 * Go to the loop below 4 times for a total of 2 sec timout.
	 * If a timeout happens due to no XTAL present or a faulty XTAL
	 * the clock system will fall back to MODCLK*/

	PJSEL1 &= ~(BIT4|BIT5);
	PJSEL0 |= BIT4|BIT5;
	/* Set HFXT pins to output to reduce power consumption */
	PJDIR |= BIT6|BIT7;
	/* Turn LFXT ON */
	CSCTL4 &= ~(LFXTOFF);

	// uint16_t timeout = 0x4;
	uint16_t timeout = 0xFF;
	do {
		timeout--;
		/* Clear Oscillator fault flags */
		CSCTL5 &= (HFXTOFFG + LFXTOFFG);
		/* Clear the Oscillator fault interrupt flag */
		SFRIFG1 &= ~OFIFG;
		/* @ 1MHz startup: delay for 500ms */
		delay_cycles(500000L * (F_CPU/1000000L));
		// __delay_cycles(500000L * (F_CPU/1000000L));			//intrinsic function
		if(!timeout) break;
//		__no_operation();
	/* Test the fault flag */
	}while (SFRIFG1 & OFIFG);
	// __delay_cycles(F_CPU*20);
	/* Reduce drive strength to reduce power consumption */
	CSCTL4 &= ~(LFXTDRIVE_3);
}


void initClocks(){

     CSCTL0_H = CSKEY_H;                     // Unlock CS registers

     /* Changing CPU freq. using DCOFSEL, can also change with DIVM*/
#if F_CPU >= 16000000L
	 CSCTL1 = DCORSEL | DCOFSEL_4;							//16MHz
	 CSCTL2 |= SELA__LFXTCLK|SELM__DCOCLK|SELS__DCOCLK;		//Select clock sources
	 CSCTL3 = DIVA__1 | DIVM__1 | DIVS__1;					//Select Dividers
	 __delay_cycles(F_CPU*2);
	 CSCTL5 &= ~(HFXTOFFG + LFXTOFFG);						//Clear Fault flags
#elif F_CPU >= 8000000L
	 CSCTL1 = DCORSEL | DCOFSEL_3;							//8MHz
	 CSCTL2 |= SELA__LFXTCLK|SELM__DCOCLK|SELS__DCOCLK;		//Select clock sources
	 CSCTL3 = DIVA__1 | DIVM__1 | DIVS__1;					//Select Dividers
	 CSCTL5 &= ~(HFXTOFFG + LFXTOFFG);						//Clear Fault flags
#elif F_CPU >= 4000000L
	 CSCTL1 &= ~DCORSEL;									//4MHz
	 CSCTL1 |= DCOFSEL_3;
	 CSCTL2 |= SELA__LFXTCLK|SELM__DCOCLK|SELS__DCOCLK;		//Select clock sources
	 CSCTL3 = DIVA__1 | DIVM__1 | DIVS__1;					//Select Dividers
	 CSCTL5 &= ~(HFXTOFFG + LFXTOFFG);						//Clear Fault flags
#elif F_CPU >= 1000000L
	 CSCTL1 = DCORSEL | DCOFSEL_0;							//1MHz
	 CSCTL2 |= SELA__LFXTCLK|SELM__DCOCLK|SELS__DCOCLK;		//Select clock sources
	 CSCTL3 = DIVA__1 | DIVM__1 | DIVS__1;					//Select Dividers
	 CSCTL5 &= ~(HFXTOFFG + LFXTOFFG);						//Clear Fault flags
#else
        #warning No Suitable Frequency found!
#endif

#if defined(LOCKLPM5)
     // This part is required for FR59xx device to unlock the IOs
     PMMCTL0_H = PMMPW_H;           // open PMM
	 PM5CTL0 &= ~LOCKLPM5;          // clear lock bit for ports
     PMMCTL0_H = 0;                 // lock PMM
#endif

    // __delay_cycles(F_CPU*5);
	/* Attempt to enable the 32kHz XTAL */
	enableXtal();
	CSCTL0_H = 0;                           // Lock CS registers
}

/*******************Old Code but Modified*******************************/

///**
// * @file
// * Initialize MSP430F5529 clocks
// *
// * Modified from wiring.c from energia project under LGPLv2.1
// * Also see http://pastebin.com/LBBh0zqS
// */
//#include <msp430.h>
//#include "global.h"
//
//#pragma GCC diagnostic push
//#pragma GCC diagnostic ignored "-Wstrict-aliasing"
///* From http://mspgcc.sourceforge.net/manual/c1408.html */
//static void __inline__ delay_cycles(unsigned long n)
//{
////    RTCCTL01 = RTCHOLD | RTCSSEL0| RTCTEV1|RTCTEV0;
////#define RTCNT (*((volatile uint32_t*) &RTCNT1))
////    RTCNT = 0;
////    RTCCTL01 &= ~RTCHOLD; //Start clock
////    while(RTCNT < n);
////    RTCCTL01 |= RTCHOLD; //Stop clock
//
//    //new CODE
//   	RTCCTL0_H = RTCKEY_H;                      //unlock RTC_C for writes
//   	RTCCTL13 = RTCHOLD | RTCSSEL0 | RTCTEV1|RTCTEV0;
//#define RTCNT (*((volatile uint32_t*) &RTCCNT1))
//   	RTCNT = 0;                               //reset ctr
//   	RTCCTL13 &= ~RTCHOLD;
////   	while(RTCNT < n);
//   	RTCCTL13 |= RTCHOLD;
//   	RTCCTL0_H = 0;      					//lock RTC_C again
//}
//#pragma GCC diagnostic pop
//
//static void enableXtal()
//{
//	/* LFXT can take up to 1000ms to start.
//	 * Go to the loop below 4 times for a total of 2 sec timout.
//	 * If a timeout happens due to no XTAL present or a faulty XTAL
//	 * the clock system will fall back to MODCLK*/
////	P5SEL |= BIT4 + BIT5;
//	PJSEL1 &= ~(BIT4|BIT5);
//	PJSEL0 |= BIT4|BIT5;
//	/* Set XTAL2 pins to output to reduce power consumption */
////	P5DIR |= BIT2 + BIT3;
//	PJDIR |= BIT5|BIT4;
//	/* Turn XT1 ON */
////	UCSCTL6 &= ~(XT1OFF);
//	CSCTL4 &= ~(LFXTOFF);
//	/* Turn XT2 ON */
////	UCSCTL6 &= ~(XT2OFF);
//	/* Set XTAL CAPS to 12 pF */
////	UCSCTL6 |= XCAP_3;
//
//	uint16_t timeout = 0x4;
//	do {
//		timeout--;
//		/* Clear Oscillator fault flags */
////		UCSCTL7 &= ~(XT2OFFG + XT1LFOFFG + DCOFFG);
//		CSCTL5 &= (HFXTOFFG + LFXTOFFG);
//		/* Clear the Oscillator fault interrupt flag */
//		SFRIFG1 &= ~OFIFG;
//		/* @ 1MHz startup: delay for 500ms */
//		delay_cycles(500000L * (F_CPU/1000000L));
////		__delay_cycles(500000L * (F_CPU/1000000L));
//		if(!timeout) break;
//	/* Test the fault flag */
//		__no_operation();
//	}while (SFRIFG1 & OFIFG);
//	/* Reduce drive strength to reduce power consumption */
////	UCSCTL6 &= ~(XT1DRIVE_3);
//	CSCTL4 &= ~(LFXTDRIVE_3);
//}
//
//
//void initClocks(){
////     PMMCTL0_H = PMMPW_H;             // open PMM
////	 SVSMLCTL &= ~SVSMLRRL_7;         // reset
////	 PMMCTL0_L = PMMCOREV_0;          //
////
////	 PMMIFG &= ~(SVSMLDLYIFG|SVMLVLRIFG|SVMLIFG);  // clear flags
////	 SVSMLCTL = (SVSMLCTL & ~SVSMLRRL_7) | SVSMLRRL_1;
////	 while ((PMMIFG & SVSMLDLYIFG) == 0); // wait till settled
////	 while ((PMMIFG & SVMLIFG) == 0); // wait for flag
////	 PMMCTL0_L = (PMMCTL0_L & ~PMMCOREV_3) | PMMCOREV_1; // set VCore for lext Speed
////	 while ((PMMIFG & SVMLVLRIFG) == 0);  // wait till level reached
////
////	 PMMIFG &= ~(SVSMLDLYIFG|SVMLVLRIFG|SVMLIFG);  // clear flags
////	 SVSMLCTL = (SVSMLCTL & ~SVSMLRRL_7) | SVSMLRRL_2;
////	 while ((PMMIFG & SVSMLDLYIFG) == 0); // wait till settled
////	 while ((PMMIFG & SVMLIFG) == 0); // wait for flag
////	 PMMCTL0_L = (PMMCTL0_L & ~PMMCOREV_3) | PMMCOREV_2; // set VCore for lext Speed
////	 while ((PMMIFG & SVMLVLRIFG) == 0);  // wait till level reached
////
////	 PMMIFG &= ~(SVSMLDLYIFG|SVMLVLRIFG|SVMLIFG);  // clear flags
////	 SVSMLCTL = (SVSMLCTL & ~SVSMLRRL_7) | SVSMLRRL_3;
////	 while ((PMMIFG & SVSMLDLYIFG) == 0); // wait till settled
////	 while ((PMMIFG & SVMLIFG) == 0); // wait for flag
////	 PMMCTL0_L = (PMMCTL0_L & ~PMMCOREV_3) | PMMCOREV_3; // set VCore for lext Speed
////	 while ((PMMIFG & SVMLVLRIFG) == 0);  // wait till level reached
////	 SVSMHCTL &= ~(SVMHE+SVSHE);         // Disable High side SVS
////	 SVSMLCTL &= ~(SVMLE+SVSLE);         // Disable Low side SVS
////
////     PMMCTL0_H = 0;                   // lock PMM
//
//     CSCTL0_H = CSKEY_H;                     // Unlock CS registers
////     UCSCTL0 = 0;                     // set lowest Frequency
//#if F_CPU >= 25000000L
////     UCSCTL1 = DCORSEL_6;             //Range 6
////     UCSCTL2 = FLLD_1|0x176;          //Loop Control Setting
////	 UCSCTL3 = SELREF__XT1CLK;        //REFO for FLL
////	 UCSCTL4 = SELA__XT1CLK|SELM__DCOCLK|SELS__DCOCLK;  //Select clock sources
////	 UCSCTL7 &= ~(0x07);               //Clear Fault flags
////#elif F_CPU >= 24000000L
////     UCSCTL1 = DCORSEL_6;             //Range 6
////     UCSCTL2 = FLLD_1|0x16D;          //Loop Control Setting
////	 UCSCTL3 = SELREF__XT1CLK;        //REFO for FLL
////	 UCSCTL4 = SELA__XT1CLK|SELM__DCOCLK|SELS__DCOCLK;  //Select clock sources
////	 UCSCTL7 &= ~(0x07);               //Clear Fault flags
//#elif F_CPU >= 16000000L
////     UCSCTL1 = DCORSEL_5;             //Range 6
////     UCSCTL2 = FLLD_1|0x1E7;          //Loop Control Setting
////	 UCSCTL3 = SELREF__XT1CLK;        //REFO for FLL
////	 UCSCTL4 = SELA__XT1CLK|SELM__DCOCLKDIV|SELS__DCOCLKDIV;  //Select clock sources
////	 UCSCTL7 &= ~(0x07);               //Clear Fault flags
//	 CSCTL1 = DCORSEL | DCOFSEL_4;							//16MHz
//	 CSCTL2 |= SELA__LFXTCLK|SELM__DCOCLK|SELS__DCOCLK;		//Select clock sources
//	 CSCTL3 = DIVA__1 | DIVM__1 | DIVS__1;					//Select Dividers
//	 CSCTL5 &= (HFXTOFFG + LFXTOFFG);						//Clear Fault flags
////#elif F_CPU >= 12000000L
////     UCSCTL1 = DCORSEL_5;              //Range 6
////     UCSCTL2 = FLLD_1|0x16D;           //Loop Control Setting
////	 UCSCTL3 = SELREF__XT1CLK;         //REFO for FLL
////	 UCSCTL4 = SELA__XT1CLK|SELM__DCOCLKDIV|SELS__DCOCLKDIV;  //Select clock sources
////	 UCSCTL7 &= ~(0x07);               //Clear Fault flags
//#elif F_CPU >= 8000000L
////     UCSCTL1 = DCORSEL_5;             //Range 6
////     UCSCTL2 = FLLD_1|0x0F3;          //Loop Control Setting
////	 UCSCTL3 = SELREF__XT1CLK;        //REFO for FLL
////	 UCSCTL4 = SELA__XT1CLK|SELM__DCOCLKDIV|SELS__DCOCLKDIV;  //Select clock sources
////	 UCSCTL7 &= ~(0x07);               //Clear Fault flags
//	 CSCTL1 = DCORSEL | DCOFSEL_3;							//8MHz
//	 CSCTL2 |= SELA__LFXTCLK|SELM__DCOCLK|SELS__DCOCLK;		//Select clock sources
//	 CSCTL3 = DIVA__1 | DIVM__1 | DIVS__1;					//Select Dividers
//	 CSCTL5 &= (HFXTOFFG + LFXTOFFG);						//Clear Fault flags
////#elif F_CPU >= 1000000L
////     UCSCTL1 = DCORSEL_2;             //Range 6
////     UCSCTL2 = FLLD_1|0x01D;          //Loop Control Setting
////	 UCSCTL3 = SELREF__XT1CLK;        //REFO for FLL
////	 UCSCTL4 = SELA__XT1CLK|SELM__DCOCLKDIV|SELS__DCOCLKDIV;  //Select clock sources
////	 UCSCTL7 &= ~(0x07);              //Clear Fault flags
//#else
//        #warning No Suitable Frequency found!
//#endif
//
//#if defined(LOCKLPM5)
//     // This part is required for FR59xx device to unlock the IOs
//     PMMCTL0_H = PMMPW_H;           // open PMM
//	 PM5CTL0 &= ~LOCKLPM5;          // clear lock bit for ports
//     PMMCTL0_H = 0;                 // lock PMM
//#endif
//	/* Attempt to enable the 32kHz XTAL */
//	enableXtal();
//	CSCTL0_H = 0;                           // Lock CS registers
//}


