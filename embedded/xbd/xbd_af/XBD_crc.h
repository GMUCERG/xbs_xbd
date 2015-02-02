#ifndef XBD_CRC_H_
#define XBD_CRC_H_

#include <stdint.h>
#include <XBD_HAL.h>


#define PACK_CRC(crc, dest) { *((uint8_t *) dest)=(crc >> 8);  *((uint8_t *) (dest+1))=crc & 0xff; }  
#define UNPACK_CRC(src) ((uint16_t) (*((uint8_t *) src) << 8 |  *((uint8_t *) (src+1))))

uint16_t crc16_update(uint16_t crc, uint8_t a);
uint16_t crc16buffer(const uint8_t *data, uint16_t len);


#endif
