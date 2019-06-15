#include <stdint.h>
#include <strings.h>
#include "XBD_commands.h"
#include "XBD_debug.h"
#include "XBD_operation.h"
#include "XBD_OH.h"

#include "crypto_kem.h"

#ifdef I_AM_OS_BASED
static volatile uint32_t burner;
static volatile uint32_t afterburner;

static inline void schedule_delay(void){

    /*  If running on an embedded OS, sleep some time to try and get the CPU
        uninterrupted for the measurement   */
    for(afterburner=65;afterburner<5000000;afterburner++)
    {
        for(burner=13;burner<TC_VALUE_SEC;burner++)
        {
            burner=burner*1;
        }
    }
    usleep(TC_VALUE_SEC*100000);        
}
#endif

/** 
 * Executes an operation 
 *
 * All parameters passed in buffers are in network byte order
 *
 * @param parameterType The type of parameters currently in the parameter buffer
 * @param parameterBuffer First uint32_t is encryption/decryption (0/1)
 * Subsequent parameters for keypair (0):
 * []
 * Subsequent parameters for encapsulation (1):
 * [public key len (uint32_t)][public key]
 * Subsequent parameters for decapsulation (2):
 * [ciphertext len (uint32_t)][secret key len (uint32_t)][ciphertext][secret key]
 * @param parameterBufferLen Total length of parameterBuffer
 * @param resultBuffer If in encrypt mode, then just contains ciphertext, with
 * no size delimiters. Otherwise, [plaintext len (uint32_t)][secnum len(uint32_t)][plaintext][sec num]
 * @param p_stackUse pointer to a global variable to report the amount of stack used
 * @param result_len  Length of data in resultBuffer
 * @returns 0 if success or forgery detected, else errorcode from operation
 */
int32_t OH_handleExecuteRequest(uint32_t parameterType, uint8_t *parameterBuffer, size_t parameterBufferLen, uint8_t* resultBuffer, uint32_t *p_stackUse, size_t *result_len) {

    if( CRYPTO_KEM_TYPE != parameterType ) {
        XBD_DEBUG("Rec'd W-R-O-N-G EXecute req:");
        XBD_DEBUG("\nparameterType="); XBD_DEBUG_32B(parameterType);
        *(int32_t*)resultBuffer=HTONL(FAIL_TYPE_MISMATCH);
        //prepare 'FAIL' response to XBH
        //Return only size of failure code in result buffer
        return NUMBSIZE;
    }

    /* watch out for the dogs */
    XBD_startWatchDog(100);

#define KEYPAIR_PARAM_HEADER_LEN (NUMBSIZE)
#define KEYPAIR_RESULT_HEADER_LEN (NUMBSIZE+LENGSIZE*2) // public key, secret key
#define ENC_PARAM_HEADER_LEN (NUMBSIZE+LENGSIZE)        // public key
#define ENC_RESULT_HEADER_LEN (NUMBSIZE+LENGSIZE*2)     // ciphertext, session key
#define DEC_PARAM_HEADER_LEN (NUMBSIZE+LENGSIZE*2)      // secret key, ciphertext
#define DEC_RESULT_HEADER_LEN (NUMBSIZE)       			// session key
    uint32_t *param_header = (uint32_t *)parameterBuffer;
    int32_t retval;

	XBD_DEBUG("\nMode="); XBD_DEBUG_32B(param_header[0]);

    // Keypair
    if (0 == param_header[0]) {
        unsigned char *pk; // Public key
		unsigned char *sk; // Secret key

        pk = resultBuffer + KEYPAIR_RESULT_HEADER_LEN;
		sk = pk + crypto_kem_PUBLICKEYBYTES;

#ifdef I_AM_OS_BASED
        schedule_delay();
#endif

        /* Prepare Stack usage measurement */
        XBD_paintStack();
        *p_stackUse = XBD_countStack();

        /** Sends the signal "start-of-execution" to the XBH */
        XBD_sendExecutionStartSignal();

        retval = crypto_kem_keypair(pk, sk);

        /** Sends the signal "end-of-execution" to the XBH */
        XBD_sendExecutionCompleteSignal();

        /* Report Stack usage measurement */
        *p_stackUse = *p_stackUse - XBD_countStack();

		// Fill in result headers
        *(uint32_t*)(resultBuffer + NUMBSIZE) = HTONL((uint32_t)crypto_kem_PUBLICKEYBYTES);
        *(uint32_t*)(resultBuffer + NUMBSIZE + LENGSIZE) = HTONL((uint32_t)crypto_kem_SECRETKEYBYTES);

		// Fill in result length
        *result_len = KEYPAIR_RESULT_HEADER_LEN + crypto_kem_PUBLICKEYBYTES +
				crypto_kem_SECRETKEYBYTES;
    }

	// Encapsulation
	else if (0x01000000 == param_header[0]) { // Different endian???
        unsigned char *c; // Ciphertext
        unsigned char *k; // Session key

        unsigned char *pk; // Public key
        pk = (unsigned char *)NTOHL(param_header[1]);

		XBD_DEBUG("Session key len: "); XBD_DEBUG_32B(crypto_kem_BYTES); XBD_DEBUG_CHAR('\n');
		XBD_DEBUG("Public key len: "); XBD_DEBUG_32B(crypto_kem_PUBLICKEYBYTES); XBD_DEBUG_CHAR('\n');

        c = resultBuffer + ENC_RESULT_HEADER_LEN;
        k = resultBuffer + ENC_RESULT_HEADER_LEN + crypto_kem_CIPHERTEXTBYTES;

		XBD_DEBUG_BUF("Public key", pk, crypto_kem_PUBLICKEYBYTES);

#ifdef I_AM_OS_BASED
        schedule_delay();
#endif  

        /* Prepare Stack usage measurement */
        XBD_paintStack();
        *p_stackUse = XBD_countStack();

		/** Sends the signal "start-of-execution" to the XBH */
		XBD_sendExecutionStartSignal();
		XBD_DEBUG("Execution signal sent\n");

        retval = crypto_kem_enc(c, k, pk);
		XBD_DEBUG("Finished encapsulation\n");
		XBD_DEBUG_BUF("Ciphertext", c, crypto_kem_CIPHERTEXTBYTES);

        /** Sends the signal "end-of-execution" to the XBH */
        XBD_sendExecutionCompleteSignal();
		XBD_DEBUG("Execution complete signal sent\n");

        /* Report Stack usage measurement */
        *p_stackUse = *p_stackUse - XBD_countStack();

        // Fill in result headers
        *(uint32_t*)(resultBuffer + NUMBSIZE) = HTONL((uint32_t)crypto_kem_CIPHERTEXTBYTES);
        *(uint32_t*)(resultBuffer + NUMBSIZE + LENGSIZE) = HTONL((uint32_t)crypto_kem_BYTES);

        // Fill in result length
        *result_len = ENC_RESULT_HEADER_LEN + crypto_kem_CIPHERTEXTBYTES +
				crypto_kem_BYTES;
    }

	// Decapsulation
	else if (0x02000000 == param_header[0]) { // Different endian???
		unsigned char *k; // Session key

		unsigned char *sk; // Secret key
		sk = (unsigned char *)NTOHL(param_header[1]);
		unsigned char *c; // Ciphertext
		size_t clen = NTOHL(param_header[2]);

		if (clen > parameterBufferLen) {
			XBD_DEBUG("\nParameter Buffer too small");
			return FAIL_OVERFLOW;
		}

		XBD_DEBUG("Session key len: "); XBD_DEBUG_32B(crypto_kem_BYTES); XBD_DEBUG_CHAR('\n');
		XBD_DEBUG("Secret key len: "); XBD_DEBUG_32B(crypto_kem_SECRETKEYBYTES); XBD_DEBUG_CHAR('\n');

		k = resultBuffer + DEC_RESULT_HEADER_LEN;
		c = parameterBuffer + DEC_PARAM_HEADER_LEN;

		XBD_DEBUG_BUF("Secret key", sk, crypto_kem_SECRETKEYBYTES);

#ifdef I_AM_OS_BASED
        schedule_delay();
#endif  

        /* Prepare Stack usage measurement */
        XBD_paintStack();
        *p_stackUse = XBD_countStack();

        /** Sends the signal "start-of-execution" to the XBH */
        XBD_sendExecutionStartSignal();
		XBD_DEBUG("Execution signal sent\n");

        retval = crypto_kem_dec(k, c, sk);
		XBD_DEBUG("Finished decapsulation\n");
		XBD_DEBUG_BUF("Session key", k, crypto_kem_BYTES);

        /** Sends the signal "end-of-execution" to the XBH */
        XBD_sendExecutionCompleteSignal();
		XBD_DEBUG("Execution complete signal sent\n");

        /* Report Stack usage measurement */
        *p_stackUse = *p_stackUse - XBD_countStack();

        // Fill in result length
        *result_len = DEC_RESULT_HEADER_LEN + crypto_kem_BYTES;
	}
	else {
		XBD_DEBUG("Invalid Mode : "); XBD_DEBUG_32B(param_header[0]); XBD_DEBUG_CHAR('\n');
		return FAIL_DEFAULT;
	}

	// Return result
    *(int32_t*)resultBuffer=HTONL(retval);

    /* stop the watchdog */
    XBD_stopWatchDog();

    // Forgery detected, allow return though and inspect resultBuffer later
    if (retval == -1){
        return 0;
    }else{
        //Return return code
        return retval;
    }
}


