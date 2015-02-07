/*
 * XBD_Commands.h
 *
 *  Created on: 25.07.2009
 *      Author: cwb
 */

#ifndef XBD_COMMANDS_H_
#define XBD_COMMANDS_H_

#include <XBD_HAL.h>

#define XBD_PROTO_VERSION "04"


#define REVNSIZE 7
#define TIMESIZE 4
#define ADDRSIZE 4
#define SEQNSIZE 4
#define TYPESIZE 4
#define LENGSIZE 4
#define NUMBSIZE 4
#define CRC16SIZE 2



#define XBD_COMMAND_LEN 8
#define XBD_ANSWERLENG_MAX 32


#define XBD_TYPE_EBASH 0x1
// This is the interface that is required for eBASH:
//  int crypto_hash(
//       unsigned char *out,
//       const unsigned char *in,
//       unsigned long long inlen
//     )
//     {
//       ...
//       ... the code for your MD6 implementation goes here
//       ...
//       return 0;
//     }
//
// Program Parameters XBD00ppr[TYPE][LENG][CRC]
// Parameter Download XBD00pdr[SEQN][DATA][CRC]
// Once transfer is complete the parameters buffer consists of:
// u32 inlen	//no one needs 64bit input length on a mC
// u8 in[]		//array of data to be hashed



//Requests ('r') understood by bootloader
extern const char CONSTDATAAREA XBDpfr[];
extern const char CONSTDATAAREA XBDfdr[];
extern const char CONSTDATAAREA XBDsar[];
extern const char CONSTDATAAREA XBDvir[];
extern const char CONSTDATAAREA XBDtcr[];
extern const char CONSTDATAAREA XBDtrr[];
extern const char CONSTDATAAREA XBDlor[];


//OK ('o') Responses from bootloader
extern const char CONSTDATAAREA XBDpfo[];
extern const char CONSTDATAAREA XBDfdo[];
extern const char CONSTDATAAREA XBDblo[];
extern const char CONSTDATAAREA XBDtco[];
extern const char CONSTDATAAREA XBDtro[];
extern const char CONSTDATAAREA XBDloo[];


//Failed ('f') Responses from bootloader
extern const char CONSTDATAAREA XBDpff[];
extern const char CONSTDATAAREA XBDfdf[];


//Requests ('r') understood by AppFramework
extern const char CONSTDATAAREA XBDppr[];
extern const char CONSTDATAAREA XBDpdr[];
extern const char CONSTDATAAREA XBDurr[];
extern const char CONSTDATAAREA XBDrdr[];
extern const char CONSTDATAAREA XBDexr[];
extern const char CONSTDATAAREA XBDccr[];
extern const char CONSTDATAAREA XBDsbr[];
extern const char CONSTDATAAREA XBDtcr[];
extern const char CONSTDATAAREA XBDsur[];


//OK ('o') Responses from AppFramework
extern const char CONSTDATAAREA XBDppo[];
extern const char CONSTDATAAREA XBDpdo[];
extern const char CONSTDATAAREA XBDuro[];
extern const char CONSTDATAAREA XBDrdo[];
extern const char CONSTDATAAREA XBDexo[];
extern const char CONSTDATAAREA XBDcco[];
extern const char CONSTDATAAREA XBDafo[];
extern const char CONSTDATAAREA XBDtco[];
extern const char CONSTDATAAREA XBDsuo[];



//Failed ('f') Responses from AppFramework
extern const char CONSTDATAAREA XBDppf[];
extern const char CONSTDATAAREA XBDpdf[];
extern const char CONSTDATAAREA XBDexf[];
extern const char CONSTDATAAREA XBDccf[];
extern const char CONSTDATAAREA XBDurf[];




//Unknown command response
extern const char CONSTDATAAREA XBDunk[];
//CRC wrong response
extern const char CONSTDATAAREA XBDcrc[];
#endif /* XBD_COMMANDS_H_ */
