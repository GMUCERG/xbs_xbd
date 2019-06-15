/*
 * Brian Hession
 *
 * Just tests the algorithm.
 *
 */

#include "XBD_HAL.h"
#include "XBD_commands.h"
#include "XBD_crc.h"
#include "XBD_debug.h"
#include "XBD_util.h"
#include "XBD_version.h"

#include "randombytes.h"
#include "crypto_sign.h"
#include "xbdkey.h"

extern int crypto_sign(unsigned char *,unsigned long long *,const unsigned char *,
		unsigned long long,const unsigned char *);
extern unsigned char xbdkey[];

#define MESSAGE "I love cryptography!"
#define MLEN (sizeof(MESSAGE))
#define SMLEN (crypto_sign_BYTES + sizeof(MESSAGE))

int main(void) {

	XBD_init();
	XBD_DEBUG("STANDALONE XBD DEBUG "XBX_REVISION" started\r\n");

	unsigned char sm[SMLEN];
	unsigned char m[] = MESSAGE;
	unsigned long long smlen;

	XBD_DEBUG("Message: "MESSAGE"\n");
	XBD_DEBUG("Signing...\n");
	if (crypto_sign(sm, &smlen, m, MLEN, xbdkey) != 0) {
		// Error
		while (1) {}
	}
	XBD_DEBUG_BUF("Signature", sm, smlen);

	while (1) {}

	// Not reachable
	return 0;
}

void FRW_msgRecHand(uint8_t len, uint8_t* data) {}
uint8_t FRW_msgTraHand(uint8_t maxlen, uint8_t* data) {
	return 0;
}

