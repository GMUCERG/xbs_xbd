/**
 * @file
 * Initialize MSP430F5529 clocks
 *
 * Modified from wiring.c from energia project under LGPLv2.1
 * Also see http://pastebin.com/LBBh0zqS
 */
#include <msp430.h>
#include "global.h"


static void enableXtal()
{
	/* LFXT can take up to 1000ms to start.
	 * Go to the loop below 4 times for a total of 2 sec timout.
	 * If a timeout happens due to no XTAL present or a faulty XTAL
	 * the clock system will fall back to REFOCLK (~32kHz) */
	P5SEL |= BIT4 + BIT5;
	/* Set XTAL2 pins to output to reduce power consumption */
	P5DIR |= BIT2 + BIT3;
	/* Turn XT1 ON */
	UCSCTL6 &= ~(XT1OFF);
	/* Set XTAL CAPS to 12 pF */
	UCSCTL6 |= XCAP_3;

	uint16_t timeout = 0x4;
	do {
		timeout--;
		/* Clear Oscillator fault flags */
		UCSCTL7 &= ~(XT2OFFG + XT1LFOFFG + DCOFFG);
		/* Clear the Oscillator fault interrupt flag */
		SFRIFG1 &= ~OFIFG;
		/* @ 1MHz startup: delay for 500ms */
		__delay_cycles(500000L * (F_CPU/1000000L));
		if(!timeout) break;
	/* Test the fault flag */
	}while (SFRIFG1 & OFIFG);
	/* Reduse drive strength to reduce power consumption */
	UCSCTL6 &= ~(XT1DRIVE_3);
}


void initClocks(){
     PMMCTL0_H = PMMPW_H;             // open PMM
	 SVSMLCTL &= ~SVSMLRRL_7;         // reset
	 PMMCTL0_L = PMMCOREV_0;          //
	 
	 PMMIFG &= ~(SVSMLDLYIFG|SVMLVLRIFG|SVMLIFG);  // clear flags
	 SVSMLCTL = (SVSMLCTL & ~SVSMLRRL_7) | SVSMLRRL_1;
	 while ((PMMIFG & SVSMLDLYIFG) == 0); // wait till settled
	 while ((PMMIFG & SVMLIFG) == 0); // wait for flag
	 PMMCTL0_L = (PMMCTL0_L & ~PMMCOREV_3) | PMMCOREV_1; // set VCore for lext Speed
	 while ((PMMIFG & SVMLVLRIFG) == 0);  // wait till level reached

	 PMMIFG &= ~(SVSMLDLYIFG|SVMLVLRIFG|SVMLIFG);  // clear flags
	 SVSMLCTL = (SVSMLCTL & ~SVSMLRRL_7) | SVSMLRRL_2;
	 while ((PMMIFG & SVSMLDLYIFG) == 0); // wait till settled
	 while ((PMMIFG & SVMLIFG) == 0); // wait for flag
	 PMMCTL0_L = (PMMCTL0_L & ~PMMCOREV_3) | PMMCOREV_2; // set VCore for lext Speed
	 while ((PMMIFG & SVMLVLRIFG) == 0);  // wait till level reached

	 PMMIFG &= ~(SVSMLDLYIFG|SVMLVLRIFG|SVMLIFG);  // clear flags
	 SVSMLCTL = (SVSMLCTL & ~SVSMLRRL_7) | SVSMLRRL_3;
	 while ((PMMIFG & SVSMLDLYIFG) == 0); // wait till settled
	 while ((PMMIFG & SVMLIFG) == 0); // wait for flag
	 PMMCTL0_L = (PMMCTL0_L & ~PMMCOREV_3) | PMMCOREV_3; // set VCore for lext Speed
	 while ((PMMIFG & SVMLVLRIFG) == 0);  // wait till level reached
	 SVSMHCTL &= ~(SVMHE+SVSHE);         // Disable High side SVS 
	 SVSMLCTL &= ~(SVMLE+SVSLE);         // Disable Low side SVS

     PMMCTL0_H = 0;                   // lock PMM

     UCSCTL0 = 0;                     // set lowest Frequency
#if F_CPU >= 25000000L
     UCSCTL1 = DCORSEL_6;             //Range 6
     UCSCTL2 = FLLD_1|0x176;          //Loop Control Setting
	 UCSCTL3 = SELREF__XT1CLK;        //REFO for FLL
	 UCSCTL4 = SELA__XT1CLK|SELM__DCOCLK|SELS__DCOCLK;  //Select clock sources
	 UCSCTL7 &= ~(0x07);               //Clear Fault flags
#elif F_CPU >= 24000000L
     UCSCTL1 = DCORSEL_6;             //Range 6
     UCSCTL2 = FLLD_1|0x16D;          //Loop Control Setting
	 UCSCTL3 = SELREF__XT1CLK;        //REFO for FLL
	 UCSCTL4 = SELA__XT1CLK|SELM__DCOCLK|SELS__DCOCLK;  //Select clock sources
	 UCSCTL7 &= ~(0x07);               //Clear Fault flags
#elif F_CPU >= 16000000L
     UCSCTL1 = DCORSEL_5;             //Range 6
     UCSCTL2 = FLLD_1|0x1E7;          //Loop Control Setting
	 UCSCTL3 = SELREF__XT1CLK;        //REFO for FLL
	 UCSCTL4 = SELA__XT1CLK|SELM__DCOCLKDIV|SELS__DCOCLKDIV;  //Select clock sources
	 UCSCTL7 &= ~(0x07);               //Clear Fault flags
#elif F_CPU >= 12000000L
     UCSCTL1 = DCORSEL_5;              //Range 6
     UCSCTL2 = FLLD_1|0x16D;           //Loop Control Setting
	 UCSCTL3 = SELREF__XT1CLK;         //REFO for FLL
	 UCSCTL4 = SELA__XT1CLK|SELM__DCOCLKDIV|SELS__DCOCLKDIV;  //Select clock sources
	 UCSCTL7 &= ~(0x07);               //Clear Fault flags
#elif F_CPU >= 8000000L
     UCSCTL1 = DCORSEL_5;             //Range 6
     UCSCTL2 = FLLD_1|0x0F3;          //Loop Control Setting
	 UCSCTL3 = SELREF__XT1CLK;        //REFO for FLL
	 UCSCTL4 = SELA__XT1CLK|SELM__DCOCLKDIV|SELS__DCOCLKDIV;  //Select clock sources
	 UCSCTL7 &= ~(0x07);               //Clear Fault flags
#elif F_CPU >= 1000000L
     UCSCTL1 = DCORSEL_2;             //Range 6
     UCSCTL2 = FLLD_1|0x01D;          //Loop Control Setting
	 UCSCTL3 = SELREF__XT1CLK;        //REFO for FLL
	 UCSCTL4 = SELA__XT1CLK|SELM__DCOCLKDIV|SELS__DCOCLKDIV;  //Select clock sources
	 UCSCTL7 &= ~(0x07);              //Clear Fault flags
#else
        #warning No Suitable Frequency found!
#endif

#if defined(LOCKLPM5)
     // This part is required for FR59xx device to unlock the IOs
     PMMCTL0_H = PMMPW_H;           // open PMM
	 PM5CTL0 &= ~LOCKLPM5;          // clear lock bit for ports
     PMMCTL0_H = 0;                 // lock PMM
#endif
	/* Attempt to enable the 32kHz XTAL */
	enableXtal();
}


