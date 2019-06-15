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
#include "crypto_kem.h"
#include "xbdkey.h"

extern int crypto_kem_dec(unsigned char *,const unsigned char *,const unsigned char *);
extern unsigned char xbdkey[];

#define CLEN crypto_kem_CIPHERTEXTBYTES
#define KLEN crypto_kem_BYTES

int main(void) {

	XBD_init();
	XBD_DEBUG("STANDALONE XBD DEBUG "XBX_REVISION" started\r\n");

	unsigned char c[CLEN];
	unsigned char k[KLEN];

	XBD_DEBUG("Encrypting...\n");
	if (crypto_kem_enc(c, k, xbdkey) != 0) {
		// Error
		while (1) {}
	}
	XBD_DEBUG_BUF("Ciphertext", c, CLEN);
	XBD_DEBUG_BUF("Session key", k, KLEN);

	while (1) {}

	// Not reachable
	return 0;
}

void FRW_msgRecHand(uint8_t len, uint8_t* data) {}
uint8_t FRW_msgTraHand(uint8_t maxlen, uint8_t* data) {
	return 0;
}

