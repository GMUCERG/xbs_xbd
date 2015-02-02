#ifndef XBD_CHECKSUM_H
#define XBD_CHECKSUM_H

/** 
* Computes a checksum for an hash algorithm
*	@param messageBuf temporary buffer, must be CHECKSUM_BYTES +1 large
* @param hashBuf the buffer to store the final checksum, must by crypto_hash_BYTES large
* @returns 0 if successful, errorcode else
*/
unsigned char checksum_compute_hash(unsigned char *messageBuf, unsigned char *hashBuf);


#endif
