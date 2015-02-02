/*
 * XBD_Commands.c
 *
 *  Created on: 25.07.2009
 *      Author: cwb
 */

#include "XBD_commands.h"


//Requests ('r') understood by bootloader
const char XBDpfr[] CONSTDATAAREA = "XBD"XBD_PROTO_VERSION"pfr";
const char XBDfdr[] CONSTDATAAREA = "XBD"XBD_PROTO_VERSION"fdr";
const char XBDsar[] CONSTDATAAREA = "XBD"XBD_PROTO_VERSION"sar";
const char XBDvir[] CONSTDATAAREA = "XBD"XBD_PROTO_VERSION"vir";
const char XBDtcr[] CONSTDATAAREA = "XBD"XBD_PROTO_VERSION"tcr";
const char XBDtrr[] CONSTDATAAREA = "XBD"XBD_PROTO_VERSION"trr";
const char XBDlor[] CONSTDATAAREA = "XBD"XBD_PROTO_VERSION"lor";

//OK ('o') Responses from bootloader
const char XBDpfo[] CONSTDATAAREA = "XBD"XBD_PROTO_VERSION"pfo";
const char XBDfdo[] CONSTDATAAREA = "XBD"XBD_PROTO_VERSION"fdo";
const char XBDblo[] CONSTDATAAREA = "XBD"XBD_PROTO_VERSION"BLo";
const char XBDtco[] CONSTDATAAREA = "XBD"XBD_PROTO_VERSION"tco";
const char XBDtro[] CONSTDATAAREA = "XBD"XBD_PROTO_VERSION"tro";
const char XBDloo[] CONSTDATAAREA = "XBD"XBD_PROTO_VERSION"loo";



//Failed ('f') Responses from bootloader
const char XBDpff[] CONSTDATAAREA = "XBD"XBD_PROTO_VERSION"pff";
const char XBDfdf[] CONSTDATAAREA = "XBD"XBD_PROTO_VERSION"fdf";




//Requests ('r') understood by AppFramework
const char CONSTDATAAREA XBDppr[]  = "XBD"XBD_PROTO_VERSION"ppr";
const char CONSTDATAAREA XBDpdr[]  = "XBD"XBD_PROTO_VERSION"pdr";
const char CONSTDATAAREA XBDurr[]  = "XBD"XBD_PROTO_VERSION"urr";
const char CONSTDATAAREA XBDrdr[]  = "XBD"XBD_PROTO_VERSION"rdr";
const char CONSTDATAAREA XBDexr[]  = "XBD"XBD_PROTO_VERSION"exr";
const char CONSTDATAAREA XBDccr[]  = "XBD"XBD_PROTO_VERSION"ccr";
const char CONSTDATAAREA XBDsbr[]  = "XBD"XBD_PROTO_VERSION"sbr";
const char CONSTDATAAREA XBDsur[]  = "XBD"XBD_PROTO_VERSION"sur";


//OK ('o') Responses from application
const char CONSTDATAAREA XBDppo[]  = "XBD"XBD_PROTO_VERSION"ppo";
const char CONSTDATAAREA XBDpdo[]  = "XBD"XBD_PROTO_VERSION"pdo";
const char CONSTDATAAREA XBDuro[]  = "XBD"XBD_PROTO_VERSION"uro";
const char CONSTDATAAREA XBDrdo[]  = "XBD"XBD_PROTO_VERSION"rdo";
const char CONSTDATAAREA XBDexo[]  = "XBD"XBD_PROTO_VERSION"exo";
const char CONSTDATAAREA XBDcco[]  = "XBD"XBD_PROTO_VERSION"cco";
const char CONSTDATAAREA XBDafo[]  = "XBD"XBD_PROTO_VERSION"AFo";
const char CONSTDATAAREA XBDsuo[]  = "XBD"XBD_PROTO_VERSION"suo";

//Failed ('f') Responses from bootloader
const char CONSTDATAAREA XBDppf[]  = "XBD"XBD_PROTO_VERSION"ppf";
const char CONSTDATAAREA XBDpdf[]  = "XBD"XBD_PROTO_VERSION"pdf";
const char CONSTDATAAREA XBDexf[]  = "XBD"XBD_PROTO_VERSION"exf";
const char CONSTDATAAREA XBDccf[]  = "XBD"XBD_PROTO_VERSION"ccf";
const char CONSTDATAAREA XBDurf[]  = "XBD"XBD_PROTO_VERSION"urf";




//Unknown command response
const char CONSTDATAAREA XBDunk[]  = "XBD"XBD_PROTO_VERSION"unk";
//CRC wrong response
const char CONSTDATAAREA XBDcrc[]  = "XBD"XBD_PROTO_VERSION"crc";


