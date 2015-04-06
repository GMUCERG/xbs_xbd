#include <stdint.h>
#include <strings.h>
#include "crypto_hash.h"
#include "XBD_commands.h"
#include "XBD_debug.h"
#include "XBD_operation.h"
#include "XBD_OH.h"
#include "try.h"

volatile uint32_t burner;
volatile uint32_t afterburner;


int32_t OH_handleChecksumRequest(uint8_t *parameterBuffer, uint8_t *resultBuffer, uint32_t *p_stackUse, size_t *result_len) {
    int retval;

	/* wacht out for the dogs */
	XBD_startWatchDog(550);
	/* Prepare Stack usage measurement */
	XBD_paintStack();
	*p_stackUse=XBD_countStack();
	
    test_allocate(parameterBuffer);
    test_reset();
    // Set pointer to write error message to
    try_errmsg_buf = resultBuffer+NUMBSIZE;

	/* Sends the signal "start-of-execution" to the XBH */
	XBD_sendExecutionStartSignal();

    //Get return value in network byte order 
    retval = test();
	
	/** Sends the signal "end-of-execution" to the XBH */
	XBD_sendExecutionCompleteSignal();
	
	/* Report Stack usage measurement */
	*p_stackUse=*p_stackUse-XBD_countStack();

	/* stop the watchdog */
	XBD_stopWatchDog();

    // If try_failed is true, error has occured
    // Currently don't report why failed other than via serial port
    //Big endian format, so 0 pad first
    *(int32_t*)resultBuffer=HTONL(retval);

    if(retval == FAIL_CHECKSUM){
        // Return value + message string + null terminator
        *result_len = NUMBSIZE+strlen(resultBuffer+4)+1;
    }else if (retval == 0){
        // Copy back first 32 bytes of checksum state
        memcpy(resultBuffer+4, checksum_state, CHECKSUM_LEN);
        *result_len = NUMBSIZE+CHECKSUM_LEN;
    }else{
        *result_len = NUMBSIZE;
    }

	
	#ifdef XBX_DEBUG
	XBD_DEBUG_BUF("resultBuffer", resultBuffer, NUMBSIZE+CHECKSUM_LEN);
    #endif

	//prepare 'OK' response to XBH
    return retval;
}
