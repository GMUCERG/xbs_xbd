#include "XBD_crc.h"

uint16_t crc16_update(uint16_t crc, uint8_t a)
{
    int i;

    crc ^= a;
    for (i = 0; i < 8; ++i)
    {
        if (crc & 1)
            crc = (crc >> 1) ^ 0xA001;
        else
            crc = (crc >> 1);
    }
    return crc;
}


uint16_t crc16buffer(const uint8_t *data, uint16_t len) {
	uint16_t crc = 0xffff, ctr;

	for (ctr = 0; ctr < len; ++ctr) {
		crc = crc16_update(crc, data[ctr]);
	}
	
	return crc;
}
