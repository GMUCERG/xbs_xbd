#include "crypto_aead.h"

// Needed for lengths
#include "XBD_commands.h"

#define CRYPTO_AEAD_TYPE 0x2
#define CAESAR_TYPE CRYPTO_AEAD_TYPE
#define XBD_OPERATION_TYPE CRYPTO_AEAD_TYPE


// Total size of AD + MSG will be under 2048
#define MAX_MSG_BYTES 2048
#define MAX_AD_BYTES MAX_MSG_BYTES

#define MAX_TEST_MSG_BYTES 128
#define MAX_TEST_AD_BYTES MAX_TEST_MSG_BYTES
#define MAX_ABYTES 128
#define MAX_CBYTES (MAX_MSG+MAX_ABYTES)
#define MAX_NSECBYTES 64
#define MAX_NPUBBYTES 64
#define MAX_KEYBYTES 64

// Need to reserve 16 bytes before and after each paramter as a canary
// (len(k+s+p+a+m+c+t+r)+(number of parameters*2*16))*2
#define MAX_TESTBUFFER_BYTES (((MAX_KEYBYTES+MAX_NSECBYTES+MAX_NPUBBYTES+MAX_TEST_AD_BYTES+MAX_TEST_MSG_BYTES+(MAX_TEST_MSG_BYTES+MAX_ABYTES)*2+MAX_NSECBYTES)+16*8*2)*2)

// Maximum parameter size during runs
// Length of parameters + integer for direction
#define MAX_PARAM_BYTES (LENGSIZE*5+NUMBSIZE+(MAX_KEYBYTES+MAX_NSECBYTES+MAX_MSG_BYTES+MAX_TEST_AD_BYTES+MAX_NPUBBYTES))

#define MAX_ENCRYPT_OUTPUT_BYTES (NUMBSIZE+MAX_MSG_BYTES+MAX_ABYTES)
#define MAX_DECRYPT_OUTPUT_BYTES (NUMBSIZE+LENGSIZE*2+MAX_MSG_BYTES+MAX_NSECBYTES)

#if crypto_aead_ABYTES > MAX_ABYTES
#error ABYTES too large!
#endif

#if crypto_aead_NSECBYTES > MAX_NSECBYTES
#error NSECBYTES too large!
#endif

#if crypto_aead_NPUBBYTES > MAX_NPUBBYTES
#error NPUBBYTES too large!
#endif

#if crypto_aead_KEYBYTES > MAX_KEYBYTES
#error KEYBYTES too large!
#endif

// Calculate PARAMLENG 
// Parameter buffer is also used for TEST
#define XBD_PARAMLENG_MAX (MAX_TESTBUFFER_BYTES > MAX_PARAM_BYTES ? MAX_TESTBUFFER_BYTES:MAX_PARAM_BYTES)
#define XBD_RESULTLEN (MAX_ENCRYPT_OUTPUT_BYTES > MAX_DECRYPT_OUTPUT_BYTES ? MAX_ENCRYPT_OUTPUT_BYTES : MAX_DECRYPT_OUTPUT_BYTES)
