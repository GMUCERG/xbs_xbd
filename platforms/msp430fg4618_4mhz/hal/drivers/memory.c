#include <msp430fg4618.h>
#include "msp430libtypes.h" 

void WriteFlash( unsigned int *uintAddress, unsigned int uintData)
{
	//WDTCTL = WDTPW+0x0c;
	asm(" dint");	//	Interrupts disabled
	while(BUSY & FCTL3);		//	Wait until flash ready
	FCTL3 = FWKEY;	//	Clear lock bits (LOCK & LOCKA)
	FCTL1 = FWKEY + WRT;	//	Enable byte/word write mode
	u16 u;
	*uintAddress = uintData;
	FCTL1 = FWKEY;
	FCTL3 ^= (FXKEY + LOCK);
	asm(" eint");
}
