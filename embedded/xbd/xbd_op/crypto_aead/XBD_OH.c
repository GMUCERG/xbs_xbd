#include <stdint.h>
#include <strings.h>
#include "crypto_aead.h"
#include "XBD_commands.h"
#include "XBD_debug.h"
#include "XBD_operation.h"
#include "XBD_OH.h"

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
 * Subsequent parameters for encryption (0):
 * [plaintext len (uint32_t)][assoc data len (uint32_t)][secret number len(uint32_t)][public number len(uint32_t)][key len (uint32_t)][plaintext][assoc data][secret number][public number][key]
 * Subsequent parameters for decryption (1):
 * [ciphertext len (uint32_t)][assoc data len (uint32_t)][public number len(uint32_t)][key len (uint32_t)][ciphertext][assoc data][public number][key]
 * @param parameterBufferLen Total length of parameterBuffer
 * @param resultBuffer If in encrypt mode, then just contains ciphertext, with
 * no size delimiters. Otherwise, [plaintext len (uint32_t)][secnum len(uint32_t)][plaintext][sec num]
 * @param p_stackUse pointer to a global variable to report the amount of stack used
 * @param result_len  Length of data in resultBuffer
 * @returns 0 if success or forgery detected, else errorcode from operation
 */
int32_t OH_handleExecuteRequest(uint32_t parameterType, uint8_t *parameterBuffer, size_t parameterBufferLen, uint8_t* resultBuffer, uint32_t *p_stackUse, size_t *result_len) {

    if( CRYPTO_AEAD_TYPE != parameterType ) {
        XBD_DEBUG("Rec'd W-R-O-N-G EXecute req:");
        XBD_DEBUG("\nparameterType="); XBD_DEBUG_32B(parameterType);
        *(int32_t*)resultBuffer=HTONL(FAIL_TYPE_MISMATCH);
        //prepare 'FAIL' response to XBH
        //Return only size of failure code in result buffer
        return NUMBSIZE;
    }

    /* watch out for the dogs */
    XBD_startWatchDog(100);

#define ENC_PARAM_HEADER_LEN (NUMBSIZE+LENGSIZE*5)
#define ENC_RESULT_HEADER_LEN (NUMBSIZE)
#define DEC_PARAM_HEADER_LEN (NUMBSIZE+LENGSIZE*4)
#define DEC_RESULT_HEADER_LEN (NUMBSIZE+LENGSIZE*2)
    uint32_t *param_header = (uint32_t *)parameterBuffer;
    int32_t retval;

    //Encrypt
    if(0 == param_header[0]){
        unsigned char *c; //Ciphertext
        unsigned long long clen;

        unsigned char *m; //plaintext Message
        size_t mlen = NTOHL(param_header[1]);
        unsigned char *a; //Assoc data
        size_t adlen = NTOHL(param_header[2]);
        unsigned char *s; //Secret number
        size_t slen = NTOHL(param_header[3]);
        unsigned char *p; //Public number
        size_t plen = NTOHL(param_header[4]);
        unsigned char *k; //Key
        size_t klen = NTOHL(param_header[5]);

        if(mlen+adlen+slen+plen+klen > parameterBufferLen){
            return FAIL_OVERFLOW;
        }

        c = resultBuffer+ENC_RESULT_HEADER_LEN;

        m = parameterBuffer+ENC_PARAM_HEADER_LEN;
        a = m+mlen;
        s = a+adlen;
        p = s+slen;
        k = p+plen;

#ifdef I_AM_OS_BASED
        schedule_delay();
#endif  

        /* Prepare Stack usage measurement */
        XBD_paintStack();
        *p_stackUse=XBD_countStack();

        /** Sends the signal "start-of-execution" to the XBH */
        XBD_sendExecutionStartSignal();

        retval = crypto_aead_encrypt(c, &clen, m, mlen, a, adlen, s, p, k);

        /** Sends the signal "end-of-execution" to the XBH */
        XBD_sendExecutionCompleteSignal();

        /* Report Stack usage measurement */
        *p_stackUse=*p_stackUse-XBD_countStack();
        *result_len = ENC_RESULT_HEADER_LEN+(size_t)clen;
    }else{
        unsigned char *m; //plaintext Message
        unsigned long long mlen;
        unsigned char *s; //Secret number
        //size_t slen = crypto_aead_NSECBYTES;

        unsigned char *c; //Ciphertext
        size_t clen = NTOHL(param_header[1]);
        unsigned char *a; //Assoc data
        size_t adlen = NTOHL(param_header[2]);
        unsigned char *p; //Public number
        size_t plen = NTOHL(param_header[3]);
        unsigned char *k; //Key
        size_t klen = NTOHL(param_header[4]);


        if(clen+adlen+plen+klen > parameterBufferLen){
            return FAIL_OVERFLOW;
        }


        //Secret number goes first since it is fixed length, and length can be
        //determined before running decrypt. 
        s = resultBuffer+DEC_RESULT_HEADER_LEN;
        m = resultBuffer+DEC_RESULT_HEADER_LEN+crypto_aead_NSECBYTES;

        c = parameterBuffer+DEC_PARAM_HEADER_LEN;
        a = c+clen;
        p = a+adlen;
        k = p+plen;

#ifdef I_AM_OS_BASED
        schedule_delay();
#endif  

        /* Prepare Stack usage measurement */
        XBD_paintStack();
        *p_stackUse=XBD_countStack();

        /** Sends the signal "start-of-execution" to the XBH */
        XBD_sendExecutionStartSignal();

        retval = crypto_aead_decrypt(m, &mlen, s, c, clen, a, adlen, p, k);

        /** Sends the signal "end-of-execution" to the XBH */
        XBD_sendExecutionCompleteSignal();

        /* Report Stack usage measurement */
        *p_stackUse=*p_stackUse-XBD_countStack();

        //Fill in result headers
        *(uint32_t*)(resultBuffer+NUMBSIZE)=HTONL((uint32_t)crypto_aead_NSECBYTES);
        *(uint32_t*)(resultBuffer+NUMBSIZE+LENGSIZE)=HTONL((uint32_t)mlen);

        //Fill in result length
        *result_len = DEC_RESULT_HEADER_LEN + crypto_aead_NSECBYTES + (size_t)mlen;
    }

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


