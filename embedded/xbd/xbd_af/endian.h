#ifndef XBX_ENDIAN_H
#define XBX_ENDIAN_H

/* some files need an endian definition */

#include "XBD_DeviceDependent.h"

#define LITTLE_ENDIAN 1234
#define __LITLE_ENDIAN LITTLE_ENDIAN

#define BIG_ENDIAN 4321
#define __BIG_ENDIAN BIG_ENDIAN

#ifdef XBX_LITTLE_ENDIAN
  #define BYTE_ORDER LITTLE_ENDIAN
  #define NESSIE_LITTLE_ENDIAN
  #ifdef XBX_BIG_ENDIAN
    #error BOTH XBX_LITTLE_ENDIAN AND XBX_BIG_ENDIAN SPEICIFIED! 
  #endif

#else 
  #ifdef XBX_BIG_ENDIAN
    #define BYTE_ORDER BIG_ENDIAN
    #define NESSIE_BIG_ENDIAN
  #else 
    #errror NEITHER XBX_LITTLE_ENDIAN NOR XBX_BIG_ENDIAN SPECIFIED IN XBD_DEVICEDEPENDENT.H!
  #endif
#endif


#define __BYTE_ORDER BYTE_ORDER


#endif
