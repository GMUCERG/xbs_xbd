#include "crypto_sign.h"

// Needed for lengths
#include "XBD_commands.h"

#define CRYPTO_SIGN_TYPE 0x4
#define SIGN_TYPE CRYPTO_SIGN_TYPE
#define XBD_OPERATION_TYPE CRYPTO_SIGN_TYPE

// Total size of PubKey + K + PrivKey + C will be under 8192
#define MAX_MSG_BYTES       512
#define MAX_PUBLICKEYBYTES 2976
#define MAX_SECRETKEYBYTES 4160
#define MAX_BYTES          8080

// Need to reserve 16 bytes before and after each paramter as a canary
// (len(p+s+k+c+t)+(number of parameters*2*16))*2
#ifndef XBD_NOCHECKSUM
#define MAX_TESTBUFFER_BYTES ((((MAX_MSG_BYTES*3)+MAX_PUBLICKEYBYTES+MAX_SECRETKEYBYTES+(MAX_BYTES*2))+(5*16*2)))
#else
#define MAX_TESTBUFFER_BYTES 0
#endif

// Maximum parameter size during runs
// Length of parameters + integer for direction
#define MAX_KEYPAIR_PARAM_BYTES (NUMBSIZE)
#define MAX_SIGN_PARAM_BYTES ( \
	NUMBSIZE + (LENGSIZE * 2) + \
	MAX_MSG_BYTES)
#define MAX_OPEN_PARAM_BYTES ( \
	NUMBSIZE + (LENGSIZE * 2) + \
	MAX_MSG_BYTES + MAX_BYTES)

/*
#define MAX_KEYPAIR_OUTPUT_BYTES ( \
	NUMBSIZE + (LENGSIZE * 2) + \
	MAX_PUBLICKEYBYTES + \
	MAX_SECRETKEYBYTES)
*/
#define MAX_SIGN_OUTPUT_BYTES ( \
	NUMBSIZE + \
	MAX_BYTES + MAX_MSG_BYTES)
#define MAX_OPEN_OUTPUT_BYTES ( \
	NUMBSIZE + \
	MAX_MSG_BYTES)

#if crypto_sign_PUBLICKEYBYTES > MAX_PUBLICKEYBYTES
#error PUBLICKEYBYTES too large!
#endif

#if crypto_sign_SECRETKEYBYTES > MAX_SECRETKEYBYTES
#error SECRETKEYBYTES too large!
#endif

#if crypto_sign_BYTES > MAX_BYTES
#error BYTES too large!
#endif

// Get the bigger param size
#define MAX_PARAM_BYTES ( \
	MAX_SIGN_PARAM_BYTES > MAX_OPEN_PARAM_BYTES \
	? MAX_SIGN_PARAM_BYTES : MAX_OPEN_PARAM_BYTES)

// Calculate PARAMLENG 
// Parameter buffer is also used for TEST
#define XBD_PARAMLENG_MAX ( \
	MAX_TESTBUFFER_BYTES > MAX_PARAM_BYTES \
	? MAX_TESTBUFFER_BYTES : MAX_PARAM_BYTES)

// Get the bigger output size
/*
#define MAX_OUTPUT_BYTES ( \
	MAX_KEYPAIR_OUTPUT_BYTES > MAX_SIGN_OUTPUT_BYTES \
	? MAX_KEYPAIR_OUTPUT_BYTES : MAX_SIGN_OUTPUT_BYTES)
*/
#define XBD_RESULTLEN ( \
	MAX_SIGN_OUTPUT_BYTES > MAX_OPEN_OUTPUT_BYTES \
	? MAX_SIGN_OUTPUT_BYTES : MAX_OPEN_OUTPUT_BYTES)

