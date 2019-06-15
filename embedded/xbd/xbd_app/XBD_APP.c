#include <stdalign.h>
#include <string.h>

#include "XBD_APP.h"
#include "XBD_HAL.h"
#include "XBD_OH.h"
#include "XBD_operation.h"
#include "XBD_commands.h"
#include "XBD_crc.h"
#include "XBD_debug.h"
#include "XBD_multipacket.h"

uint8_t XBD_response[XBD_COMMAND_LEN+1];
// Upload Results XBD00urr[TYPE][DATA][CRC]
// The results in the DATA part are:
// u8 returncode	//the hash function return code 0=success
// u8 padding[3]        // zero padding to make out aligned
// u8 out[]			//the hash result
//


#define XBD_RESULTBUF_LENG (XBD_COMMAND_LEN+XBD_RESULTLEN+CRC16SIZE)

/* we do that to get aligned buffers */
static alignas(sizeof(uint32_t)) uint8_t xbd_parameter_buffer[XBD_PARAMLENG_MAX];
static alignas(sizeof(uint32_t)) uint8_t xbd_result_buffer[XBD_RESULTBUF_LENG];
static alignas(sizeof(uint32_t)) uint8_t xbd_answer_buffer[XBD_ANSWERLENG_MAX];

static struct xbd_multipkt_state multipkt_state;
static size_t result_buffer_len;
static uint32_t xbd_stack_use;



#ifndef DEVICE_SPECIFIC_SANE_TC_VALUE
	#define DEVICE_SPECIFIC_SANE_TC_VALUE 30000
#endif

typedef enum enum_XBD_State {
	fresh = 0, paramdownload, paramok, executed, reporting, reportuploaded, checksummed
} XBD_State;

XBD_State xbd_state = fresh;

// These are used by XBD_AF_MsgRecHand and its sub-functions
char buf[8+1];
uint16_t txDataLen; //I2C transmit size not including CRC16
uint16_t ctr, crc, rx_crc;

/**
Results Packet 	 rp 	 XBD00rpo[TYPE][ADDR][LENG][CRC]
Results Data 	rd 	XBD00rdo[SEQN][DATA][CRC]
 */




void XBD_AF_DisregardBlock(uint8_t len, uint8_t *data) {
	//disregard block
	XBD_loadStringFromConstDataArea((char *)XBD_response, XBDcrc);
	XBD_DEBUG((char *)XBD_response);
	XBD_DEBUG(" :");
	for (ctr = 0; ctr < len - CRC16SIZE; ++ctr) {
		if (0 == ctr % 16)
			XBD_DEBUG_CHAR('\n');
		XBD_DEBUG_BYTE(data[ctr]);
	}
	XBD_DEBUG_CHAR('\n');
	XBD_DEBUG_BYTE(crc >> 8); XBD_DEBUG_BYTE(crc & 0xff); XBD_DEBUG_CHAR('_');
	XBD_DEBUG_BYTE(rx_crc >> 8); XBD_DEBUG_BYTE(rx_crc & 0xff);
	XBD_DEBUG("\n---------------------\n");

	txDataLen=XBD_COMMAND_LEN;
	return;
}

void XBD_AF_HandleProgramParametersRequest(uint8_t len, uint8_t* data) {
    uint8_t retval;
    XBD_loadStringFromConstDataArea(buf, XBDppr);
    retval = XBD_recInitialMultiPacket(&multipkt_state, data, len, buf, true, true);

#ifdef XBX_DEBUG_APP
	XBD_DEBUG("\ntype="); XBD_DEBUG_32B(multipkt_state.type);
	XBD_DEBUG("\naddr="); XBD_DEBUG_32B(multipkt_state.addr);
	XBD_DEBUG("\nleng="); XBD_DEBUG_32B(multipkt_state.dataleft);
	XBD_DEBUG("\ndataptr="); XBD_DEBUG_32B((uint32_t)data);
	XBD_DEBUG("\ndataptr+offset="); XBD_DEBUG_32B((uint32_t)(data + XBD_COMMAND_LEN + ADDRSIZE + TYPESIZE));
#endif

    if(retval){
        //prepare 'OK' response to XBH
        txDataLen=XBD_COMMAND_LEN;
        XBD_loadStringFromConstDataArea((char *)XBD_response, XBDppf);
        return;
    }

    //Handle case of empty 0 byte packet
    if(multipkt_state.dataleft == 0){
        xbd_state = paramok;
    }else{
        xbd_state = paramdownload;
    }

    //prepare 'OK' response to XBH
    txDataLen=XBD_COMMAND_LEN;
    XBD_loadStringFromConstDataArea((char *)XBD_response, XBDppo);
	return;
}

void XBD_AF_HandleParameterDownloadRequest(uint8_t len, uint8_t* data) {
    XBD_DEBUG("XBDpdr");

    if (xbd_state == paramdownload){

        int8_t ret = XBD_recSucessiveMultiPacket(&multipkt_state, data, len,
                xbd_parameter_buffer+multipkt_state.addr,
                XBD_PARAMLENG_MAX-multipkt_state.addr, XBDpdr);
        if(ret){
            XBD_DEBUG("Rec'd W-R-O-N-G Program Parameters Req");
            XBD_loadStringFromConstDataArea((char *)XBD_response, XBDpdf);
            txDataLen=XBD_COMMAND_LEN;
            return;
        }else{
            if(multipkt_state.dataleft == 0){
                xbd_state = paramok;
            }
            txDataLen=XBD_COMMAND_LEN;
            XBD_loadStringFromConstDataArea((char *)XBD_response, XBDpdo);
            return;
        }
    }
    XBD_DEBUG("Not in parameter download state");
    XBD_loadStringFromConstDataArea((char *)XBD_response, XBDpdf);
    txDataLen=XBD_COMMAND_LEN;
}

    

void XBD_AF_HandleUploadResultsRequest(uint8_t len, uint8_t* data) {
    XBD_DEBUG("XBDurr");
	if( (xbd_state == executed) || ( checksummed == xbd_state )) {
		//prepare 'OK' response to XBH
        XBD_loadStringFromConstDataArea(buf, XBDuro);
		XBD_genInitialMultiPacket(&multipkt_state, xbd_result_buffer, result_buffer_len, xbd_answer_buffer, buf, NO_MP_ADDR, XBD_OPERATION_TYPE);
        xbd_state = reporting;
        txDataLen = XBD_ANSWERLENG_MAX-CRC16SIZE;
        return;
	} else {
		XBD_DEBUG("Rec'd W-R-O-N-G UploadResults req:");

		XBD_DEBUG("\nxbd_state="); XBD_DEBUG_32B(xbd_state);
		//prepare 'FAIL' response to XBH
		XBD_loadStringFromConstDataArea((char *)XBD_response, XBDurf);
		txDataLen=XBD_COMMAND_LEN;
        return;
	}


}

void XBD_AF_HandleResultDataRequest(uint8_t len, uint8_t* data) {
    if( xbd_state == reporting )
    {
        //prepare 'OK' response to XBH
        XBD_loadStringFromConstDataArea(buf, XBDrdo);
        XBD_genSucessiveMultiPacket(&multipkt_state, xbd_answer_buffer, XBD_ANSWERLENG_MAX-CRC16SIZE, buf);
        txDataLen = XBD_ANSWERLENG_MAX-CRC16SIZE;

        if(0 == multipkt_state.dataleft)
            xbd_state = reportuploaded;

        XBD_DEBUG("\nxbd_genmp_dataleft="); XBD_DEBUG_32B(multipkt_state.dataleft);

#ifdef XBX_DEBUG_APP_VERBOSE
        XBD_DEBUG("\nxbd_result_buffer:");
        for (ctr = 0; ctr < XBD_RESULTLEN; ++ctr) {
            if (0 == ctr % 16)
                XBD_DEBUG_CHAR('\n');
            XBD_DEBUG_BYTE(xbd_result_buffer[ctr]);
        }
        XBD_DEBUG("\n--------");
#endif
    } else {
        XBD_DEBUG("Rec'd W-R-O-N-G UploadResults req:");
        XBD_DEBUG("\nxbd_state="); XBD_DEBUG_32B(xbd_state);
        //prepare 'FAIL' response to XBH
        XBD_loadStringFromConstDataArea((char *)XBD_response, XBDurf);
        txDataLen=XBD_COMMAND_LEN;
    }

}

void XBD_AF_HandleEXecuteRequest(uint8_t len, uint8_t* data) {

	if( (xbd_state == paramok || xbd_state == executed) )
	{
#ifdef XBX_DEBUG_APP
		XBD_DEBUG("Rec'd good EXecute req:");
#endif
		
        size_t xbd_parameter_buffer_len = multipkt_state.addr+multipkt_state.datanext;
		XBD_DEBUG("\nParameter Length: "); XBD_DEBUG_32B(xbd_parameter_buffer_len);
		XBD_DEBUG_CHAR('\n');
			
        int32_t ret = OH_handleExecuteRequest(
                multipkt_state.type,
                xbd_parameter_buffer,
                xbd_parameter_buffer_len,
                xbd_result_buffer,
                &xbd_stack_use,
                &result_buffer_len);

#ifdef XBX_DEBUG_APP
		XBD_DEBUG("OH_handleExecuteRequest ret="); XBD_DEBUG_BYTE(ret);
		XBD_DEBUG("\nOH_handleExecuteRequest stack use="); XBD_DEBUG_32B(xbd_stack_use);
		XBD_DEBUG_CHAR('\n');
#endif

		xbd_state = executed;

		if(0 == ret )
		{
			//prepare 'OK' response to XBH
			XBD_loadStringFromConstDataArea(buf, XBDexo);
			XBD_DEBUG("Prepared 'OK' response\n");
		}
		else
		{
			//prepare 'FAIL' response to XBH
			XBD_loadStringFromConstDataArea(buf, XBDexf);
			XBD_DEBUG("Prepared 'FAIL' response\n");
		}
	} 
	else
	{
		XBD_DEBUG("Rec'd W-R-O-N-G EXecute req:");
		XBD_DEBUG("\nxbd_state="); XBD_DEBUG_32B(xbd_state);
		//prepare 'FAIL' response to XBH
		XBD_loadStringFromConstDataArea(buf, XBDexf);
	}

#ifdef XBX_DEBUG_APP
	XBD_DEBUG_CHAR('\n');
	XBD_DEBUG(buf);
	XBD_DEBUG_CHAR('\n');
#endif
	txDataLen=XBD_COMMAND_LEN;
	strcpy((char *) XBD_response, buf);
	return;
}

void XBD_AF_HandleChecksumComputeRequest(uint8_t len, uint8_t* data) {

#ifdef XBX_DEBUG_APP
    XBD_DEBUG("Rec'd good CC req:");
#endif

    int32_t ret = OH_handleChecksumRequest(
            xbd_parameter_buffer,
            xbd_result_buffer,
            &xbd_stack_use, 
            &result_buffer_len);
    

    xbd_state = checksummed;

#ifdef XBX_DEBUG_APP
    XBD_DEBUG("\nOH_handleCCRequest ret="); XBD_DEBUG_BYTE(ret);
    XBD_DEBUG("\nOH_handleCCRequest stack use="); XBD_DEBUG_32B(xbd_stack_use);
#endif

#ifdef XBX_DEBUG_APP_VERBOSE
    XBD_DEBUG("\nxbd_result_buffer:");
    for (ctr = 0; ctr < XBD_RESULTLEN; ++ctr) {
        if (0 == ctr % 16)
            XBD_DEBUG_CHAR('\n');
        XBD_DEBUG_BYTE(xbd_result_buffer[ctr]);
    }
    XBD_DEBUG("\n--------");
#endif

	if(ret == 0) {
		//prepare 'OK' response to XBH
		XBD_loadStringFromConstDataArea(buf, XBDcco);
	} else {
		//prepare 'FAIL' response to XBH
		XBD_loadStringFromConstDataArea(buf, XBDccf);
	}
	 
#ifdef XBX_DEBUG_APP
    XBD_DEBUG_CHAR('\n');
    XBD_DEBUG(buf);
    XBD_DEBUG_CHAR('\n');
#endif

    txDataLen=XBD_COMMAND_LEN;
    strcpy((char *)XBD_response, (char *)buf);
    return;

}

void XBD_AF_HandleStartBootloaderRequest() {
	XBD_switchToBootLoader();
}

void XBD_AF_HandleVersionInformationRequest() {
	txDataLen=XBD_COMMAND_LEN;
	XBD_loadStringFromConstDataArea((char *)XBD_response, XBDafo);
}

void XBD_AF_HandleTimingCalibrationRequest() {
	uint32_t elapsed = XBD_busyLoopWithTiming(DEVICE_SPECIFIC_SANE_TC_VALUE);
	XBD_loadStringFromConstDataArea((char *)xbd_answer_buffer, XBDtco);
	*((uint32_t*) (xbd_answer_buffer + XBD_COMMAND_LEN)) = HTONL(elapsed);
	txDataLen=XBD_COMMAND_LEN+NUMBSIZE;
	xbd_state = reportuploaded;
}

void XBD_AF_HandleStackUsageRequest() {
	xbd_stack_use &= 0x7fffffff;
	XBD_loadStringFromConstDataArea((char *)xbd_answer_buffer, XBDsuo);
	*((uint32_t*) (xbd_answer_buffer + XBD_COMMAND_LEN)) = HTONL( (uint32_t)xbd_stack_use );
	xbd_stack_use |= 0x80000000;
	txDataLen=XBD_COMMAND_LEN+NUMBSIZE;
}

void FRW_msgRecHand(uint8_t len, uint8_t* data) {

	uint8_t dataLen=len-CRC16SIZE;

#ifdef XBX_DEBUG_APP
	//output length of received block
	XBD_DEBUG("AF Rec'd len=");
	XBD_DEBUG_BYTE(len);
	XBD_DEBUG_CHAR('.');
	XBD_DEBUG_CHAR('\n');
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
		XBD_DEBUG("Finished EXecution\n");
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
    XBD_DEBUG((char *) XBD_response);
    XBD_DEBUG(": [");
    XBD_DEBUG((char *) data);
    XBD_DEBUG("]\n");
#endif
}

uint8_t FRW_msgTraHand(uint8_t maxlen, uint8_t* data) {
	XBD_DEBUG("\nMessage transmit handler\n");
	XBD_DEBUG("Size: "); XBD_DEBUG_32B(maxlen); XBD_DEBUG_CHAR('\n');
#ifdef XBX_DEBUG_APP
		uint16_t ctr;
#endif
    if (maxlen > XBD_ANSWERLENG_MAX)
        maxlen = XBD_ANSWERLENG_MAX;
	
	if(maxlen <= CRC16SIZE) {
	  XBD_DEBUG("MsgTraHand: Maxlen too small: ");
	  XBD_DEBUG_BYTE(maxlen);
	  XBD_DEBUG("\n");
	}


	if( xbd_state == reporting  || xbd_state == reportuploaded) {
		if(xbd_state == reportuploaded)
			xbd_state = fresh;


#ifdef XBX_DEBUG_APP_VERBOSE
		XBD_DEBUG("\nxbd_answer_buffer:");
		for (ctr = 0; ctr < maxlen; ++ctr) {
			if (0 == ctr % 16)
				XBD_DEBUG_CHAR('\n');
			XBD_DEBUG_BYTE(xbd_answer_buffer[ctr]);
		}
		XBD_DEBUG("\n--------");
#endif
		memcpy(data, xbd_answer_buffer, maxlen-CRC16SIZE);
	}
    else if ( (xbd_state == executed  || xbd_state == checksummed) && 
            (xbd_stack_use & 0x80000000)) {
        xbd_stack_use &= 0x7fffffff;	

#ifdef XBX_DEBUG_APP_VERBOSE
        XBD_DEBUG("\nxbd_answer_buffer:");
        for (ctr = 0; ctr < maxlen; ++ctr) {
            if (0 == ctr % 16)
                XBD_DEBUG_CHAR('\n');
            XBD_DEBUG_BYTE(xbd_answer_buffer[ctr]);
        }
        XBD_DEBUG("\n--------");
#endif
        memcpy(data, xbd_answer_buffer, maxlen-CRC16SIZE);
    } else {
        strncpy((char*) data, (char *)XBD_response, XBD_COMMAND_LEN);
    }

    crc = crc16buffer(data, txDataLen);
    uint8_t *target=data+txDataLen; 
    PACK_CRC(crc,target);	
#ifdef XBX_DEBUG_APP
    // do not remove this debug out, there's a timing problem otherwise with i2c
    XBD_DEBUG_BUF("FRW_msgTraHand", data, txDataLen);
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
			XBD_DEBUG("\n### Endian issue: XBX_LITTLE_ENDIAN but big endian detected! ###\n");
		#endif
	}
	else	//we are little endian
	{
		#ifdef XBX_BIG_ENDIAN
			XBD_DEBUG("\n### Endian issue: XBX_BIG_ENDIAN but little endian detected! ###\n");
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
        //usart_puts("in XBP APP\n\r");
	XBD_AF_EndianCheck();
	XBD_DEBUG("XBD APP started\n");

	while(1)
	{
		XBD_serveCommunication();
	}

}
