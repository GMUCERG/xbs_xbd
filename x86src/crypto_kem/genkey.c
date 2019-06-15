/*
 * Brian Hession
 *
 * Generates keys
 */

#include <stdio.h>
#include <b64/cencode.h>
#include "crypto_kem.h"

extern int crypto_kem_keypair(unsigned char *,unsigned char *);

#define plen crypto_kem_PUBLICKEYBYTES
#define slen crypto_kem_SECRETKEYBYTES

int b64encode(unsigned char *code_out, unsigned char *data_in, size_t data_len) {
	int codelength = 0;
	unsigned char *pos = code_out;

	base64_encodestate state;
	base64_init_encodestate(&state);
	codelength = base64_encode_block(data_in, data_len, pos, &state);
	if (codelength < 0)
		return codelength;
	pos += codelength;
	codelength = base64_encode_blockend(pos, &state);
	if (codelength < 0)
		return codelength;
	pos += codelength;
	*pos = 0;
	return 0;
}

int main(void)
{
	unsigned char p[plen];
	unsigned char s[slen];
	char pcode[plen*2];
	char scode[slen*2];
	if (crypto_kem_keypair(p,s) != 0)
		return -1;

	if (b64encode(pcode, p, plen) != 0 || b64encode(scode, s, slen) != 0)
		return -1;

	printf("%s\n", pcode);
	printf("%s", scode);
}
