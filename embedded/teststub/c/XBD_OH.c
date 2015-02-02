#include <stdint.h>

uint8_t OH_handleExecuteRequest(uint32_t parameterType, uint8_t *parameterBuffer, uint8_t* resultBuffer, uint32_t *p_stackUse) {
		return 1;
}

uint8_t OH_handleChecksumRequest(uint8_t *parameterBuffer, uint8_t* resultBuffer, uint32_t *p_stackUse) {
	
	return 0;

}
