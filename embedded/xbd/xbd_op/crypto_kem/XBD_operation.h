#include "crypto_kem.h"

// Needed for lengths
#include "XBD_commands.h"

#define CRYPTO_KEM_TYPE 0x3
#define KEM_TYPE CRYPTO_KEM_TYPE
#define XBD_OPERATION_TYPE CRYPTO_KEM_TYPE

#define MAX_PUBLICKEYBYTES  1824
#define MAX_SECRETKEYBYTES  3680
#define MAX_BYTES             48
#define MAX_CIPHERTEXTBYTES 2208

// Need to reserve 16 bytes before and after each paramter as a canary
// (len(p+s+k+c+t)+(number of parameters*2*16))*2
#ifndef XBD_NOCHECKSUM
#define MAX_TESTBUFFER_BYTES (((MAX_PUBLICKEYBYTES+MAX_SECRETKEYBYTES+MAX_CIPHERTEXTBYTES+(MAX_BYTES*2))+(16*5*2)))
#else
#define MAX_TESTBUFFER_BYTES 0
#endif

/**
 * MAX_TESTBUFFER_BYTES cannot be too large.The board halts when 
 * accessing memory after a certain point... Maybe whatever the
 * page size or cache is?
 */
#define MAX_WORKING_BUFFER (1<<14)
#if MAX_TESTBUFFER_BYTES > MAX_WORKING_BUFFER
#error MAX_TESTBUFFER_BYTES is too large!
#endif

// Maximum parameter size during runs
// Length of parameters + integer for direction
#define MAX_KEYPAIR_PARAM_BYTES (NUMBSIZE)
#define MAX_ENC_PARAM_BYTES (NUMBSIZE+LENGSIZE)
#define MAX_DEC_PARAM_BYTES (NUMBSIZE+LENGSIZE*2+MAX_CIPHERTEXTBYTES)
#define MAX_PARAM_BYTES (MAX_ENC_PARAM_BYTES > MAX_DEC_PARAM_BYTES ? MAX_ENC_PARAM_BYTES:MAX_DEC_PARAM_BYTES)

#define MAX_KEYPAIR_OUTPUT_BYTES (NUMBSIZE+LENGSIZE*2+MAX_PUBLICKEYBYTES+MAX_SECRETKEYBYTES)
#define MAX_ENC_OUTPUT_BYTES (NUMBSIZE+LENGSIZE*2+MAX_CIPHERTEXTBYTES+MAX_BYTES)
#define MAX_DEC_OUTPUT_BYTES (NUMBSIZE+MAX_BYTES)

#if crypto_kem_PUBLICKEYBYTES > MAX_PUBLICKEYBYTES
#error PUBLICKEYBYTES too large!
#endif

#if crypto_kem_SECRETKEYBYTES > MAX_SECRETKEYBYTES
#error SECRETKEYBYTES too large!
#endif

#if crypto_kem_BYTES > MAX_BYTES
#error BYTES too large!
#endif

#if crypto_kem_CIPHERTEXTBYTES > MAX_CIPHERTEXTBYTES
#error CIPHERTEXTBYTES too large!
#endif

// Calculate PARAMLENG 
// Parameter buffer is also used for TEST
#define XBD_PARAMLENG_MAX (MAX_TESTBUFFER_BYTES > MAX_PARAM_BYTES ? MAX_TESTBUFFER_BYTES:MAX_PARAM_BYTES)
#define XBD_RESULTLEN (MAX_ENC_OUTPUT_BYTES > MAX_DEC_OUTPUT_BYTES ? MAX_ENC_OUTPUT_BYTES:MAX_DEC_OUTPUT_BYTES)
