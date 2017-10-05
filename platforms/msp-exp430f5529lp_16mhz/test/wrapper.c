#include <msp430.h> 
#include "crypto_aead.h"

/*
 * main.c
 */
int error = 0;


int main(void) {
	static unsigned char k[128];
	static unsigned char c[128+32];
	static unsigned char ad[128];
	static unsigned char m[128];
	static unsigned char t[128];
	static unsigned char nsec[128];
	static unsigned char npub[128];
    unsigned long long adlen;
    unsigned long long mlen;
    unsigned long long clen;
    unsigned long long tlen;
    unsigned int i;

    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer
	
    // Disable the GPIO power-on default high-impedance mode to activate
    // previously configured port settings
    // PM5CTL0 &= ~LOCKLPM5;

    mlen=128;
    clen=128+32;
    adlen=128;
    tlen=128;

    for (i=127; i!=0; i--) k[i]=0;
    for (i=127; i!=0; i--) m[i]=i;
    for (i=127; i!=0; i--) t[i]=0;
    for (i=127; i!=0; i--) ad[i]=i;
    for (i=127; i!=0; i--) nsec[i]=i;
    for (i=127; i!=0; i--) npub[i]=i;


    crypto_aead_encrypt(c,&clen,m,mlen,ad,adlen,nsec,npub,k);

    //use 'm' instead of 't' to save memory, remember to reinitialize 'm' before decrypt!
//    for (i=127; i!=0; i--) m[i]=0;

    error = crypto_aead_decrypt(t,&mlen,nsec,c,clen,ad,adlen,npub,k);
    //check for error for decrypt

    while(1);


	return 0;
}
