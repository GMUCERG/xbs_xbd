#ifndef __XBD_OH_H__
#define __XBD_OH_H__

#include <stdint.h>
#include <sys/types.h>
#include "try.h"


int32_t OH_handleExecuteRequest( uint32_t parameterType, uint8_t * parameterBuffer, size_t parameterBufferLen, uint8_t * resultBuffer, uint32_t *p_stackUse, size_t *result_len);
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
#define FAIL_OVERFLOW -60

#endif
