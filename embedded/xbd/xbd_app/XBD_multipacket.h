#ifndef XBD_multipacket_h
#define XBD_multipacket_h

extern uint32_t xbd_genmp_dataleft;
extern uint8_t realTXlen;

uint32_t XBD_genSucessiveMultiPacket(const uint8_t* srcdata, uint8_t* dstbuf, uint32_t dstlenmax, const char  *code);
uint32_t XBD_genInitialMultiPacket(const uint8_t* srcdata, uint32_t srclen, uint8_t* dstbuf,const uint8_t *code, uint32_t type, uint32_t addr);
uint8_t XBD_recSucessiveMultiPacket(const uint8_t* recdata, uint32_t reclen, uint8_t* dstbuf, uint32_t dstlenmax, const char *code);
uint8_t XBD_recInitialMultiPacket(const uint8_t* recdata, uint32_t reclen, const char *code, uint8_t hastype, uint8_t hasaddr);

#endif
