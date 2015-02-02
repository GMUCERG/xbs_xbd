/*
 * crypto_hash/try.c version 20090118
 * D. J. Bernstein
 * Public domain.
 * modified for XBX by Jens Graef
 */

#include <stdlib.h>
#include "crypto_hash.h"
#include <XBD_debug.h>
#include <XBD_HAL.h>

#define CHECKSUM_BYTES 128
#define CHECKSUM_PAD_BYTES 2048

unsigned char checksum_compute_hash(unsigned char *hashBuf, unsigned char *messageBuf)
{
  long long i;
  long long j;
  
  unsigned char *h=hashBuf;
  unsigned char *m=messageBuf;

  #ifdef XBX_DEBUG_CHECKSUMS
  XBD_debugOut("Calculating checksum (16 dots)\n");
  #endif
  for (i = 0;i < CHECKSUM_BYTES;++i) {
    long long hlen = crypto_hash_BYTES;
    long long mlen = i;

/* hash the current buffer */
    if (crypto_hash(h,m,mlen) != 0) return 255;

    /* xor the message with the hash */
    for (j = 0;j < mlen;++j) m[j] ^= h[j % hlen];
    /* take the first byte of the hash as new next byte for message */
    m[mlen] = h[0];
    #ifdef XBX_DEBUG_CHECKSUMS
    if( (i & 7) == 7) XBD_debugOutChar('.');
    #endif
  }
  
  /** padding */
  for(i=CHECKSUM_BYTES; i<CHECKSUM_PAD_BYTES;i++)
  {
    // don't use a long long here, that confuses avr-gcc
    uint32_t counter=i-CHECKSUM_BYTES;

    /** convert to bcd */
    unsigned char high=(counter%100)/10;
    unsigned char low=counter%10;
    m[i]=high*16+low;
  } 
  
  /* hash it a final time */
  if (crypto_hash(h,m,CHECKSUM_PAD_BYTES) != 0) return 254;

  return 0;
}
