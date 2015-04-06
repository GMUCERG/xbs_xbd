#ifndef __XBD_OH_H__
#define __XBD_OH_H__

#include <stdint.h>
#include <sys/types.h>
#include "try.h"


/** 
 * Executes an operation 
 * resultBuffer contains 32 bit signed integer for err code in first 4 bytes in network byte
 * order, then buffers to send directly to XBH
 *
 * @param parameterType The type of parameters currently in the parameter buffer
 * @param parameterBuffer The buffer containing the parameters received from the XBH
 * @param resultBuffer A preallocated buffer to write the return value and operation results into
 * @param p_stackUse pointer to a global variable to report the amount of stack used
 * @param result_len  Length of data in resultBuffer
 * @returns 0 if success, else errorcode
 */
int32_t OH_handleExecuteRequest( uint32_t parameterType, uint8_t * parameterBuffer, uint8_t * resultBuffer, uint32_t *p_stackUse, size_t *result_len);
/** 
 * Executes a checksum operation 
 *
 * resultBuffer contains 32 bit signed integer for err code in first 4 bytes in network byte
 * order, then buffers to send directly to XBH
 *
 * @param parameterBuffer A buffer to be used for temporary storage - will be overwritten
 * @param resultBuffer A preallocated buffer to write the return value and checksum results into
 * @param p_stackUse poiter to a global variable to report the amount of stack used
 * @param result_len  Length of data in resultBuffer
 * @returns 0 if success, else error code
 */
int32_t OH_handleChecksumRequest(uint8_t *parameterBuffer, uint8_t *resultBuffer, uint32_t *p_stackUse, size_t *result_len);

/*
 * Possible return values inside resultBuffer are:
 *
 * FAIL_RETVAL: From try.h - test verification failure
 * FAIL_DEFAULT; Default failure code
 * FAIL_TYPE_MISMATCH: Failure if type code wrong
 *
 */
#define FAIL_DEFAULT -63
#define FAIL_TYPE_MISMATCH -61

#endif
