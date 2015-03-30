#include <stdint.h>
#include <strings.h>
#include "crypto_hash.h"
#include "XBD_commands.h"
#include "XBD_debug.h"
#include "XBD_operation.h"
#include "try.h"

volatile uint32_t burner;
volatile uint32_t afterburner;


uint8_t OH_handleChecksumRequest(uint8_t *parameterBuffer, uint8_t* resultBuffer, uint32_t *p_stackUse) {
	/* wacht out for the dogs */
	XBD_startWatchDog(550);
	/* Prepare Stack usage measurement */
	XBD_paintStack();
	*p_stackUse=XBD_countStack();
	
    test_allocate(parameterBuffer);
    test_reset();

	/** Sends the signal "start-of-execution" to the XBH */
	XBD_sendExecutionStartSignal();
    test();
	
	/** Sends the signal "end-of-execution" to the XBH */
	XBD_sendExecutionCompleteSignal();
	
	/* Report Stack usage measurement */
	*p_stackUse=*p_stackUse-XBD_countStack();

    // Copy back first 32 bytes of checksum state
    memcpy(resultBuffer+4, checksum_state, 32);

    // If try_failed is true, error has occured
    // Currently don't report why failed other than via serial port
	resultBuffer[0]=try_failed?1:0;
	resultBuffer[1]=0;
	resultBuffer[2]=0;
	resultBuffer[3]=0;
	
	/* stop the watchdog */
	XBD_stopWatchDog();
	
	#ifdef XBX_DEBUG
	XBD_DEBUG_BUF("resultBuffer", resultBuffer, crypto_hash_BYTES+1);
    #endif

	//prepare 'OK' response to XBH
	return resultBuffer[0];
}
