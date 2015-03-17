#include <stdint.h>
#include <strings.h>
#include "crypto_hash.h"
#include "XBD_commands.h"
#include "XBD_debug.h"
#include "crypto_hash_checksum.h"
#include "try.h"

volatile uint32_t burner;
volatile uint32_t afterburner;


uint8_t OH_handleExecuteRequest(uint32_t parameterType, uint8_t *parameterBuffer, uint8_t* resultBuffer, uint32_t *p_stackUse) {
	
	if( XBD_TYPE_EBASH == parameterType )
	{

		uint32_t inlen = NTOHL(*(uint32_t*)parameterBuffer);
		
		uint8_t *p_in=&parameterBuffer[sizeof(uint32_t)];

		#ifdef XBX_DEBUG_EBASH
		XBD_DEBUG("\ninlen=");XBD_DEBUG_32B(inlen);
		XBD_DEBUG_BUF("*p_in", p_in, inlen);
		#endif
                /* watch out for the dogs */
		XBD_startWatchDog(100);

		/* Prepare Stack usage measurement */
		XBD_paintStack();
		*p_stackUse=XBD_countStack();

		/*	If running on an embedded OS, sleep some time to try and get the CPU
			uninterrupted for the measurement	*/
		#ifdef I_AM_OS_BASED
                for(afterburner=65;afterburner<5000000;afterburner++)
                {
		        for(burner=13;burner<TC_VALUE_SEC;burner++)
		        {
		                burner=burner*1;
		        }
		}
			usleep(TC_VALUE_SEC*100000);		
		#endif	
		
		/** Sends the signal "start-of-execution" to the XBH */
		XBD_sendExecutionStartSignal();
		uint8_t ret = crypto_hash(
			       resultBuffer+4,
			       p_in,
			       (unsigned long long) inlen
			     );
		/** Sends the signal "end-of-execution" to the XBH */
		XBD_sendExecutionCompleteSignal();

		/* Report Stack usage measurement */
		*p_stackUse=*p_stackUse-XBD_countStack();

		resultBuffer[0]=ret;
		resultBuffer[1]=0;
		resultBuffer[2]=0;
		resultBuffer[3]=0;
		
		/* stop the watchdog */
		XBD_stopWatchDog();
		
		#ifdef XBX_DEBUG_EBASH
		XBD_DEBUG("\ncrypto_hash ret="); XBD_DEBUG_BYTE(ret);
		XBD_DEBUG_BUF("resultBuffer", resultBuffer, crypto_hash_BYTES+1);
        #endif

		//prepare 'OK' response to XBH
		return 0;
	} 
	else
	{
		XBD_DEBUG("Rec'd W-R-O-N-G EXecute req:");
		XBD_DEBUG("\nparameterType="); XBD_DEBUG_32B(parameterType);
		//prepare 'FAIL' response to XBH
		return 1;
	}
}

uint8_t OH_handleChecksumRequest(uint8_t *parameterBuffer, uint8_t* resultBuffer, uint32_t *p_stackUse) {
	/* wacht out for the dogs */
	XBD_startWatchDog(550);
	/* Prepare Stack usage measurement */
	XBD_paintStack();
	*p_stackUse=XBD_countStack();
	
    test_reset();
    test_allocate(parameterBuffer);

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
