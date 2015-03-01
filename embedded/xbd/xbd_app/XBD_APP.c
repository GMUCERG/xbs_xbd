#include <XBD_HAL.h>
#include "XBD_APP.h"
#include <XBD_debug.h>
#include <string.h>
#include <XBD_commands.h>
#include <XBD_OH.h>
#include <crypto_operation.h>
#include <XBD_crc.h>
#include "XBD_multipacket.h"

uint8_t XBD_response[XBD_COMMAND_LEN+1];

#define XBD_PARAMLENG_MAX (2048+ADDRSIZE)
#define XBD_RESULTLEN (1+3+CRYPTO_OPERATION_RESULT_DATALENG)
// Upload Results XBD00urr[TYPE][DATA][CRC]
// The results in the DATA part are:
// u8 returncode	//the hash function return code 0=success
// u8 padding[3]        // zero padding to make out aligned
// u8 out[]			//the hash result
//


#define XBD_RESULTBUF_LENG (XBD_COMMAND_LEN+XBD_RESULTLEN+CRC16SIZE)

/* we do that to get aligned buffers */
uint32_t xbd_parameter_buffer_aligned[(XBD_PARAMLENG_MAX+3)/4];
uint32_t xbd_result_buffer_aligned[(XBD_RESULTBUF_LENG+3)/4];
uint32_t xbd_answer_buffer_aligned[(XBD_ANSWERLENG_MAX+3)/4];

uint8_t *xbd_parameter_buffer=(uint8_t *)&xbd_parameter_buffer_aligned;
uint8_t *xbd_result_buffer=(uint8_t *)&xbd_result_buffer_aligned;
uint8_t *xbd_answer_buffer=(uint8_t *)&xbd_answer_buffer_aligned;

uint32_t xbd_stack_use;


#define XBD_MUPA_UNUSED 0xFFFFFFFF

#ifndef DEVICE_SPECIFIC_SANE_TC_VALUE
	#define DEVICE_SPECIFIC_SANE_TC_VALUE 30000
#endif



typedef enum enum_XBD_State {
	fresh = 0, paramdownload, paramok, executed, reporting, reportuploaded, checksummed
} XBD_State;

XBD_State xbd_state = fresh;

uint32_t xbd_parameter_type,xbd_parameter_addr, xbd_parameter_leng, xbd_parameter_seqn;
uint16_t xbd_parambuf_idx;

// These are used by XBD_AF_MsgRecHand and its sub-functions
char buf[8+1];
uint16_t ctr, crc, rx_crc;

/**
Results Packet 	 rp 	 XBD00rpo[TYPE][ADDR][LENG][CRC]
Results Data 	rd 	XBD00rdo[SEQN][DATA][CRC]
 */




void XBD_AF_DisregardBlock(uint8_t len, uint8_t *data) {
	//disregard block
	XBD_loadStringFromConstDataArea((char *)XBD_response, XBDcrc);
	XBD_debugOut((char *)XBD_response);
	XBD_debugOut(" :");
	for (ctr = 0; ctr < len - CRC16SIZE; ++ctr) {
		if (0 == ctr % 16)
			XBD_debugOutChar('\n');
		XBD_debugOutHexByte(data[ctr]);
	}
	XBD_debugOutChar('\n');
	XBD_debugOutHexByte(crc >> 8), XBD_debugOutHexByte(crc & 0xff), XBD_debugOutChar('_');
	XBD_debugOutHexByte(rx_crc >> 8), XBD_debugOutHexByte(rx_crc & 0xff);
	XBD_debugOut("\n---------------------\n");

	realTXlen=XBD_COMMAND_LEN+CRC16SIZE;
	return;
}

void XBD_AF_HandleProgramParametersRequest(uint8_t len, uint8_t* data) {
	xbd_parameter_type = NTOHL( *((uint32_t*) (data + XBD_COMMAND_LEN)) );
	xbd_parameter_addr = NTOHL( *((uint32_t*) (data + XBD_COMMAND_LEN+ ADDRSIZE)) );
	xbd_parameter_leng = NTOHL( *((uint32_t*) (data + XBD_COMMAND_LEN + ADDRSIZE + TYPESIZE)) );
	
	#ifdef XBX_DEBUG_APP
	XBD_debugOut("\ntype="), XBD_debugOutHex32Bit(xbd_parameter_type);
	XBD_debugOut("\naddr="), XBD_debugOutHex32Bit(xbd_parameter_addr);
	XBD_debugOut("\nleng="), XBD_debugOutHex32Bit(xbd_parameter_leng);
	XBD_debugOut("\ndataptr="), XBD_debugOutHex32Bit(data);
	XBD_debugOut("\ndataptr+offset="), XBD_debugOutHex32Bit(data + XBD_COMMAND_LEN + ADDRSIZE + TYPESIZE);
	#endif

	if ((XBD_TYPE_EBASH == xbd_parameter_type)
		&& (xbd_parameter_leng <= XBD_PARAMLENG_MAX) //length small enough
	) {
		
		/*
		XBD_debugOut("AF Rec'd correct PP req:");
		XBD_debugOut("\nTYPE="), XBD_debugOutHex32Bit(xbd_parameter_type);
		XBD_debugOut("\nADDR="), XBD_debugOutHex32Bit(xbd_parameter_addr);
		XBD_debugOut("\nLENG="), XBD_debugOutHex32Bit(xbd_parameter_leng);
		for (ctr = 0; ctr < xbd_parameter_leng; ++ctr) {
			if (0 == ctr % 16)
				XBD_debugOutChar('\n');
			XBD_debugOutHexByte(data[ctr]);
		}
		XBD_debugOut("\n--------");
		*/
		xbd_parambuf_idx = 0;
		xbd_state = paramdownload;
		xbd_parameter_seqn = 0;

		//prepare 'OK' response to XBH
		XBD_loadStringFromConstDataArea(buf, XBDppo);

	} else {
		XBD_debugOut("Rec'd W-R-O-N-G PP req:");
		XBD_debugOut("\nTYPE="), XBD_debugOutHex32Bit(xbd_parameter_type);
		XBD_debugOut("\nLENG="), XBD_debugOutHex32Bit(xbd_parameter_leng);
		//prepare 'FAIL' response to XBH
		XBD_loadStringFromConstDataArea(buf, XBDppf);
	}

	#ifdef XBX_DEBUG_APP
	XBD_debugOutChar('\n');
	XBD_debugOut(buf);
	XBD_debugOutChar('\n');
	#endif

	realTXlen=XBD_COMMAND_LEN+CRC16SIZE;
        strcpy((char *)XBD_response, buf);
	return;
}

void XBD_AF_HandleParameterDownloadRequest(uint8_t len, uint8_t* data) {

	uint8_t cpylen = len - XBD_COMMAND_LEN - SEQNSIZE;
	#ifdef XBX_DEBUG_APP
	XBD_debugOut("\ncpylen="), XBD_debugOutHex32Bit(cpylen);
	XBD_debugOut("\nxbd_parambuf_idx="), XBD_debugOutHex32Bit(xbd_parambuf_idx);
	XBD_debugOut("\nxbd_parameter_leng="), XBD_debugOutHex32Bit(xbd_parameter_leng);				
	#endif
	if ( xbd_parameter_seqn == NTOHL( *((uint32_t*) (data + XBD_COMMAND_LEN)) ) ) 
	{
		if (xbd_state == paramdownload)
		{
			if ((xbd_parambuf_idx+cpylen)<=xbd_parameter_leng)
			{
				
				uint8_t *p_src = (void*) (data+XBD_COMMAND_LEN + SEQNSIZE);
				memcpy(xbd_parameter_buffer+xbd_parameter_addr+xbd_parambuf_idx, p_src,cpylen );
				xbd_parambuf_idx+=cpylen;

				++xbd_parameter_seqn;

				if(xbd_parambuf_idx == xbd_parameter_leng) {xbd_state = paramok;}
				/*
				XBD_debugOut("\npd data rec'd:");
				for (ctr = 0; ctr < cpylen; ++ctr) {
					if (0 == ctr % 16)
						XBD_debugOutChar('\n');
					XBD_debugOutHexByte(p_src[ctr]);
				}
				XBD_debugOut("\n--------");
				*/


				//prepare 'OK' response to XBH
				XBD_loadStringFromConstDataArea(buf, XBDpdo);
			} else {
				XBD_debugOut("Rec'd W-R-O-N-G PD #3 req:");
				XBD_debugOut("\ncpylen="), XBD_debugOutHex32Bit(cpylen);
				XBD_debugOut("\nxbd_parambuf_idx="), XBD_debugOutHex32Bit(xbd_parambuf_idx);
				XBD_debugOut("\nxbd_parameter_leng="), XBD_debugOutHex32Bit(xbd_parameter_leng);				
				XBD_debugOutBuffer("arsch", data, len);
				XBD_loadStringFromConstDataArea(buf, XBDpdf);
			}	
		} else {
			XBD_debugOut("Rec'd W-R-O-N-G PD #2 req:");
			XBD_debugOut("\nxbd_state != paramdownload ="), XBD_debugOutHex32Bit(xbd_state);
			XBD_debugOutBuffer("arsch", data, len);
			XBD_loadStringFromConstDataArea(buf, XBDpdf);

		}
	} else {
		XBD_debugOut("Rec'd W-R-O-N-G PD #1 req:");
		XBD_debugOut("\nSEQN="), XBD_debugOutHex32Bit( NTOHL( *((uint32_t*) (data + XBD_COMMAND_LEN)) ));
		XBD_debugOut("\nxbd_parameter_seqn="), XBD_debugOutHex32Bit(xbd_parameter_seqn);

		XBD_debugOut("\ncpylen="), XBD_debugOutHex32Bit(cpylen);
		XBD_debugOut("\nxbd_parambuf_idx="), XBD_debugOutHex32Bit(xbd_parambuf_idx);
		XBD_debugOut("\nxbd_parameter_leng="), XBD_debugOutHex32Bit(xbd_parameter_leng);
		
		XBD_debugOut("\nstate="), XBD_debugOutHex32Bit(xbd_state);
		XBD_debugOut("\nlen="), XBD_debugOutHex32Bit(len);
		XBD_debugOutBuffer("arsch", data, len);
		//prepare 'FAIL' response to XBH
		XBD_loadStringFromConstDataArea(buf, XBDpdf);
	}

    #ifdef XBX_DEBUG_APP
	XBD_debugOutChar('\n');
	XBD_debugOut(buf);
	XBD_debugOutChar('\n');
	#endif
	realTXlen=XBD_COMMAND_LEN+CRC16SIZE;
	strcpy((char *)XBD_response, buf);
	return;

}

void XBD_AF_HandleUploadResultsRequest(uint8_t len, uint8_t* data) {
	if( ( (XBD_TYPE_EBASH == xbd_parameter_type) && (xbd_state == executed) ) 
		|| ( checksummed == xbd_state )	 
	)
	{
		//prepare 'OK' response to XBH
		XBD_genInitialMultiPacket(xbd_result_buffer, XBD_RESULTLEN, xbd_answer_buffer,(uint8_t *) XBDuro, XBD_TYPE_EBASH, XBD_MUPA_UNUSED);
		xbd_state = reporting;
		#ifdef XBX_DEBUG_APP
		XBD_debugOut("\nxbd_result_buffer:");
		for (ctr = 0; ctr < XBD_RESULTLEN; ++ctr) {
			if (0 == ctr % 16)
				XBD_debugOutChar('\n');
			XBD_debugOutHexByte(xbd_result_buffer[ctr]);
		}
		XBD_debugOut("\n--------");
		#endif

	} else {
		XBD_debugOut("Rec'd W-R-O-N-G UploadResults req:");

		XBD_debugOut("\nxbd_state="), XBD_debugOutHex32Bit(xbd_state);
		//prepare 'FAIL' response to XBH
		XBD_loadStringFromConstDataArea(buf, XBDurf);
		realTXlen=XBD_COMMAND_LEN+CRC16SIZE;
	}

        #ifdef XBX_DEBUG_APP
	XBD_debugOutChar('\n');
	XBD_debugOut(buf);
	XBD_debugOutChar('\n');
	#endif
	strcpy((char *)XBD_response, buf);
	return;

}

void XBD_AF_HandleResultDataRequest(uint8_t len, uint8_t* data) {
	if( xbd_state == reporting )
	{
		//prepare 'OK' response to XBH
		XBD_genSucessiveMultiPacket(xbd_result_buffer,xbd_answer_buffer, XBD_ANSWERLENG_MAX-CRC16SIZE, XBDrdo);
		
		if(0 == xbd_genmp_dataleft)
			xbd_state = reportuploaded;

                #ifdef XBX_DEBUG_APP
		XBD_debugOut("\nxbd_genmp_dataleft="), XBD_debugOutHex32Bit(xbd_genmp_dataleft);

		XBD_debugOut("\nxbd_result_buffer:");
		for (ctr = 0; ctr < XBD_RESULTLEN; ++ctr) {
			if (0 == ctr % 16)
				XBD_debugOutChar('\n');
			XBD_debugOutHexByte(xbd_result_buffer[ctr]);
		}
		XBD_debugOut("\n--------");
		#endif
	} else {
		XBD_debugOut("Rec'd W-R-O-N-G UploadResults req:");
		XBD_debugOut("\nxbd_state="), XBD_debugOutHex32Bit(xbd_state);
		//prepare 'FAIL' response to XBH
		XBD_loadStringFromConstDataArea(buf, XBDurf);
		realTXlen=XBD_COMMAND_LEN+CRC16SIZE;
	}

  #ifdef XBX_DEBUG_APP
	XBD_debugOutChar('\n');
	XBD_debugOut(buf);
	XBD_debugOutChar('\n');
	#endif
	strcpy((char *)XBD_response, buf);
	return;

}

void XBD_AF_HandleEXecuteRequest(uint8_t len, uint8_t* data) {

	if( (xbd_state == paramok || xbd_state == executed) )
	{
		#ifdef XBX_DEBUG_APP
		XBD_debugOut("Rec'd good EXecute req:");
		#endif
		
			
		uint8_t ret = OH_handleExecuteRequest(
			       xbd_parameter_type,
			       xbd_parameter_buffer,
			       xbd_result_buffer,
				&xbd_stack_use );

		#ifdef XBX_DEBUG_APP
		XBD_debugOut("\nOH_handleExecuteRequest ret="), XBD_debugOutHexByte(ret);
		XBD_debugOut("\nOH_handleExecuteRequest stack use="), XBD_debugOutHex32Bit(xbd_stack_use);
		#endif

		xbd_state = executed;
		/*
		XBD_debugOut("\nxbd_result_buffer:");
		for (ctr = 0; ctr < XBD_RESULTLEN; ++ctr) {
			if (0 == ctr % 16)
				XBD_debugOutChar('\n');
			XBD_debugOutHexByte(xbd_result_buffer[ctr]);
		}
		XBD_debugOut("\n--------");
		*/
		if(0 == ret )
		{
			//prepare 'OK' response to XBH
			XBD_loadStringFromConstDataArea(buf, XBDexo);
		}
		else
		{
			//prepare 'FAIL' response to XBH
			XBD_loadStringFromConstDataArea(buf, XBDexf);
		}
	} 
	else
	{
		XBD_debugOut("Rec'd W-R-O-N-G EXecute req:");
		XBD_debugOut("\nxbd_state="), XBD_debugOutHex32Bit(xbd_state);
		//prepare 'FAIL' response to XBH
		XBD_loadStringFromConstDataArea(buf, XBDexf);
	}

	#ifdef XBX_DEBUG_APP
	XBD_debugOutChar('\n');
	XBD_debugOut(buf);
	XBD_debugOutChar('\n');
	#endif
	realTXlen=XBD_COMMAND_LEN+CRC16SIZE;
	strcpy((char *) XBD_response, buf);
	return;
}

void XBD_AF_HandleChecksumComputeRequest(uint8_t len, uint8_t* data) {
		
        #ifdef XBX_DEBUG_APP
	XBD_debugOut("Rec'd good CC req:");
	#endif
				
	uint8_t ret = OH_handleChecksumRequest(
		       xbd_parameter_buffer,
		       xbd_result_buffer,
			 &xbd_stack_use );

	xbd_state = checksummed;

        #ifdef XBX_DEBUG_APP
	XBD_debugOut("\nOH_handleCCRequest ret="), XBD_debugOutHexByte(ret);
	XBD_debugOut("\nOH_handleCCRequest stack use="), XBD_debugOutHex32Bit(xbd_stack_use);

	XBD_debugOut("\nxbd_result_buffer:");
	for (ctr = 0; ctr < XBD_RESULTLEN; ++ctr) {
		if (0 == ctr % 16)
			XBD_debugOutChar('\n');
		XBD_debugOutHexByte(xbd_result_buffer[ctr]);
	}
	XBD_debugOut("\n--------");
	#endif

	if(0 == ret )
	{
		//prepare 'OK' response to XBH
		XBD_loadStringFromConstDataArea(buf, XBDcco);
	}
	else
	{
		//prepare 'FAIL' response to XBH
		XBD_loadStringFromConstDataArea(buf, XBDccf);
	}
	 


        #ifdef XBX_DEBUG_APP
        XBD_debugOutChar('\n');
	XBD_debugOut(buf);
	XBD_debugOutChar('\n');
	#endif
	realTXlen=XBD_COMMAND_LEN+CRC16SIZE;
	strcpy((char *)XBD_response, (char *)buf);
	return;

}

void XBD_AF_HandleStartBootloaderRequest() {
	XBD_switchToBootLoader();
}

void XBD_AF_HandleVersionInformationRequest() {
	realTXlen=XBD_COMMAND_LEN+CRC16SIZE;
	XBD_loadStringFromConstDataArea((char *)XBD_response, XBDafo);
}

void XBD_AF_HandleTimingCalibrationRequest() {
	uint32_t elapsed = XBD_busyLoopWithTiming(DEVICE_SPECIFIC_SANE_TC_VALUE);
	XBD_loadStringFromConstDataArea((char *)xbd_answer_buffer, XBDtco);
	*((uint32_t*) (xbd_answer_buffer + XBD_COMMAND_LEN)) = HTONL(elapsed);
	realTXlen=XBD_COMMAND_LEN+NUMBSIZE+CRC16SIZE;
	xbd_state = reportuploaded;
}

void XBD_AF_HandleStackUsageRequest() {
	xbd_stack_use &= 0x7fffffff;
	XBD_loadStringFromConstDataArea((char *)xbd_answer_buffer, XBDsuo);
	*((uint32_t*) (xbd_answer_buffer + XBD_COMMAND_LEN)) = HTONL( (uint32_t)xbd_stack_use );
	xbd_stack_use |= 0x80000000;
	realTXlen=XBD_COMMAND_LEN+NUMBSIZE+CRC16SIZE;
}

void FRW_msgRecHand(uint8_t len, uint8_t* data) {

        uint8_t dataLen=len-CRC16SIZE;
        #ifdef XBX_DEBUG_APP
	//output length of received block
	XBD_debugOut("AF Rec'd len=");
	XBD_debugOutHexByte(len);
	XBD_debugOutChar('.');
	XBD_debugOutChar('\n');
	#endif

	//check crc and disregard block if wrong

	rx_crc = UNPACK_CRC(&data[len - CRC16SIZE]);
	crc = crc16buffer(data, len - CRC16SIZE);
	if (rx_crc != crc) {
		XBD_AF_DisregardBlock(len, data);
		return;
	}

	// p program
	// p arameters
	// r equest
	XBD_loadStringFromConstDataArea(buf, XBDppr);
	if (0 == strncmp(buf, (char*) data, XBD_COMMAND_LEN)) {
		XBD_AF_HandleProgramParametersRequest(dataLen, data);
		return;
	}

	// p arameter
	// d ownload
	// r equest
	XBD_loadStringFromConstDataArea(buf, XBDpdr);
	if (0 == strncmp(buf, (char*) data, XBD_COMMAND_LEN)) {
		XBD_AF_HandleParameterDownloadRequest(dataLen, data);
		return;
	}

	// u pload
	// r esults
	// r equest
	XBD_loadStringFromConstDataArea(buf, XBDurr);
	if (0 == strncmp(buf, (char*) data, XBD_COMMAND_LEN)) {
		XBD_AF_HandleUploadResultsRequest(dataLen, data);
		return;
	}

	// r esult
	// d ata
	// r equest
	XBD_loadStringFromConstDataArea(buf, XBDrdr);
	if (0 == strncmp(buf, (char*) data, XBD_COMMAND_LEN)) {
		XBD_AF_HandleResultDataRequest(dataLen, data);
		return;
	}

	// e
	// x ecute (algorithm to benchmark)
	// r equest
	XBD_loadStringFromConstDataArea(buf, XBDexr);
	if (0 == strncmp(buf, (char*) data, XBD_COMMAND_LEN)) {
		XBD_AF_HandleEXecuteRequest(dataLen, data);
		return;
	}

	// c hecksum
	// c ompute
	// r equest
	XBD_loadStringFromConstDataArea(buf, XBDccr);
	if (0 == strncmp(buf, (char*) data, XBD_COMMAND_LEN)) {
		XBD_AF_HandleChecksumComputeRequest(dataLen, data);
		return;
	}

	// s tart
	// b ootloader
	// r equest
	XBD_loadStringFromConstDataArea(buf, XBDsbr);
	if (0 == strncmp(buf, (char*) data, XBD_COMMAND_LEN)) {
		XBD_AF_HandleStartBootloaderRequest();
		return;
	}

	// v ersion
	// i nformation
	// r equest
	XBD_loadStringFromConstDataArea(buf, XBDvir);
	if (0 == strncmp(buf, (char*) data, XBD_COMMAND_LEN)) {
		XBD_AF_HandleVersionInformationRequest();
		return;
	}

	// t iming
	// c alibration
	// r equest
	XBD_loadStringFromConstDataArea(buf, XBDtcr);
	if (0 == strncmp(buf, (char*) data, XBD_COMMAND_LEN)) {
		XBD_AF_HandleTimingCalibrationRequest();
		return;
	}

	// s tack
	// u sage
	// r equest
	XBD_loadStringFromConstDataArea(buf, XBDsur);
	if (0 == strncmp(buf, (char*) data, XBD_COMMAND_LEN)) {
		XBD_AF_HandleStackUsageRequest();
		return;
	}
	

	//no known command recognized
	XBD_loadStringFromConstDataArea((char *) XBD_response, XBDunk);
        #ifdef XBX_DEBUG_APP
	XBD_debugOut((char *) XBD_response);
	XBD_debugOut(": [");
	XBD_debugOut((char *) data);
	XBD_debugOut("]\n");
	#endif
}

uint8_t FRW_msgTraHand(uint8_t maxlen, uint8_t* data) {
	#ifdef XBX_DEBUG_APP
		uint16_t ctr;
	#endif
	
	if(maxlen <= CRC16SIZE) {
	  XBD_debugOut("MsgTraHand: Maxlen too small: ");
	  XBD_debugOutHexByte(maxlen);
	  XBD_debugOut("\n");
	}


	if( xbd_state == reporting  || xbd_state == reportuploaded)
	{
		if(xbd_state == reportuploaded)
			xbd_state = fresh;


		#ifdef XBX_DEBUG_APP
		XBD_debugOut("\nxbd_answer_buffer:");
		for (ctr = 0; ctr < maxlen; ++ctr) {
			if (0 == ctr % 16)
				XBD_debugOutChar('\n');
			XBD_debugOutHexByte(xbd_answer_buffer[ctr]);
		}
		XBD_debugOut("\n--------");
		#endif
		memcpy(data, xbd_answer_buffer, maxlen-CRC16SIZE);
	}
	else if (   (xbd_state == executed  || xbd_state == checksummed) 
			 && (xbd_stack_use & 0x80000000)
			)
	{
		xbd_stack_use &= 0x7fffffff;	
		if (maxlen > XBD_ANSWERLENG_MAX)
			maxlen = XBD_ANSWERLENG_MAX;

		#ifdef XBX_DEBUG_APP
		XBD_debugOut("\nxbd_answer_buffer:");
		for (ctr = 0; ctr < maxlen; ++ctr) {
			if (0 == ctr % 16)
				XBD_debugOutChar('\n');
			XBD_debugOutHexByte(xbd_answer_buffer[ctr]);
		}
		XBD_debugOut("\n--------");
		#endif
		memcpy(data, xbd_answer_buffer, maxlen-CRC16SIZE);
	} else {
   	  strncpy((char*) data, (char *)XBD_response, XBD_COMMAND_LEN);
	}
	
	crc = crc16buffer(data, realTXlen-CRC16SIZE);
	uint8_t *target=data+realTXlen-CRC16SIZE; 
	PACK_CRC(crc,target);	
#ifdef XBX_DEBUG_APP
	// do not remove this debug out, there's a timing problem otherwise with i2c
        XBD_debugOutBuffer("FRW_msgTraHand", data, realTXlen);
#endif
        return maxlen;
}

void XBD_AF_EndianCheck()
{
	uint32_t checkNum=0x12345678;
	uint8_t *p_checkBytes = (uint8_t *)&checkNum;

	if(0x12 == *p_checkBytes)	//we are big endian
	{
		#ifdef XBX_LITTLE_ENDIAN
			XBD_debugOut("\n### Endian issue: XBX_LITTLE_ENDIAN but big endian detected! ###\n");
		#endif
	}
	else	//we are little endian
	{
		#ifdef XBX_BIG_ENDIAN
			XBD_debugOut("\n### Endian issue: XBX_BIG_ENDIAN but little endian detected! ###\n");
		#endif
	}

	#ifndef XBX_LITTLE_ENDIAN
		#ifndef XBX_BIG_ENDIAN
			#error No endianess defined!
		#endif
	#endif

	#ifdef XBX_LITTLE_ENDIAN
		#ifdef XBX_BIG_ENDIAN
			#error Two endianessens defined!
		#endif
	#endif
}

int main(void)
{  
	XBD_init();
	XBD_AF_EndianCheck();
	XBD_debugOut("XBD APP started\n");

	while(1)
	{
		XBD_serveCommunication();
	}

}
