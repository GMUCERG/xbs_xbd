#ifndef __XBD_OH_H__
#define __XBD_OH_H__

#include <stdint.h>


/** Executes a crypto operation 
* @param parameterType The type of parameters currently in the parameter buffer
* @param parameterBuffer The buffer containing the parameters received from the XBH
* @param resultBuffer A preallocated buffer to write the operation results into
* @param p_stackUse poiter to a global variable to report the amount of stack used
* @returns 0 in case of success, a operation specific error code else
*/
uint8_t OH_handleExecuteRequest( uint32_t parameterType, uint8_t * parameterBuffer, uint8_t * resultBuffer, uint32_t *p_stackUse );
/** Executes a checksum operation 
* @param parameterBuffer A buffer to be used for temporary storage - will be overwritten
* @param resultBuffer A preallocated buffer to write the checksum results into
* @param p_stackUse poiter to a global variable to report the amount of stack used
* @returns 0 in case of success, a operation specific error code else
*/
uint8_t OH_handleChecksumRequest(uint8_t *parameterBuffer, uint8_t* resultBuffer, uint32_t *p_stackUse );
#endif
