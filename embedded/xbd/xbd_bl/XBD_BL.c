/*
* Attention! This bootloader needs the preprocessor symbol
* BOOTLOADER
* defined in every file (e.g. for GCC used -DBOOTLOADER).
*/




#include "XBD_BL.h"
#include <XBD_HAL.h>
#include <XBD_commands.h>

#include <XBD_debug.h>
#include <string.h>
#include <XBD_crc.h>
#include <XBD_util.h>
#include <XBD_version.h>

const char XBD_Rev[] CONSTDATAAREA = XBX_REVISION;

#define XBD_ANSWER_MAXLEN (XBD_COMMAND_LEN+REVNSIZE+CRC16SIZE)
//#define XBD_ANSWER_MAXLEN 254	//for I2C maximum data length comm tests
uint32_t XBD_alignedResponse[256/sizeof(uint32_t)];
uint8_t *XBD_response = (uint8_t*)XBD_alignedResponse;

typedef enum enum_XBD_State{
	idle = 0, flash
} XBD_State;

XBD_State xbd_state = idle;

#ifndef FLASH_ADDR_MIN
	#define FLASH_ADDR_MIN 0
#endif

uint32_t sw_alignedFlashbuffer[ (PAGESIZE+3) /4 ];
uint8_t *sw_flashbuffer= (uint8_t *) sw_alignedFlashbuffer;


uint16_t sw_flash_fbidx;
uint32_t sw_flashaddr;
uint32_t sw_flashleng;
uint32_t sw_flashprocessed;
uint32_t sw_flashseqn;



// These are used by XBD_BL_MsgRecHand and its sub-functions
uint8_t buf[8+1];
uint16_t ctr, crc, rx_crc;

uint8_t realTXlen;

void XBD_BL_DisregardBlock(uint8_t len, uint8_t *data) {
	//disregard block
	XBD_loadStringFromConstDataArea((char *)XBD_response, XBDcrc);
	realTXlen=XBD_COMMAND_LEN+CRC16SIZE;

	XBD_debugOut((char *)XBD_response);
	XBD_debugOut(" :");
	for (ctr = 0; ctr < len - CRC16SIZE; ++ctr) {
		if (0 == ctr % 16)
			XBD_debugOut("\n");
		XBD_debugOutHexByte(data[ctr]);
	}
	XBD_debugOut("\n");
	XBD_debugOutHexByte(crc >> 8), XBD_debugOutHexByte(crc & 0xff), XBD_debugOutChar('_');
	XBD_debugOutHexByte(rx_crc >> 8), XBD_debugOutHexByte(rx_crc & 0xff);
	XBD_debugOut("\n---------------------\n");
	return;
}

void XBD_BL_HandleLOopbackRequest(uint8_t len, uint8_t *data) {
	uint16_t cnt;

	for(cnt=XBD_COMMAND_LEN;cnt<len;++cnt)
	{
		XBD_response[cnt]=data[cnt]+1;	
	}
	XBD_loadStringFromConstDataArea((char *)XBD_response, XBDloo);
	realTXlen=len+CRC16SIZE;
	return;
}

void XBD_BL_HandleProgramFlashRequest(uint8_t len, uint8_t *data) {
	sw_flashaddr = NTOHL(*((uint32_t*) (data + XBD_COMMAND_LEN)));
	sw_flashleng = NTOHL(*((uint32_t*) (data + XBD_COMMAND_LEN + ADDRSIZE)));

	if ((sw_flashaddr <= (FLASH_ADDR_MAX - sw_flashleng)) //start addr low enough
			&& (sw_flashaddr >= FLASH_ADDR_MIN )
			&& (sw_flashleng <= FLASH_ADDR_MAX) //length small enough
			//&& ((sw_flashaddr & PAGE_ALIGN_MASK) == sw_flashaddr) //page aligned
	) {
		//put flash programming call here
//		XBD_debugOut("Rec'd correct request to flash:");
//		XBD_debugOut("\nADDR="), XBD_debugOutHex32Bit(sw_flashaddr);
//		XBD_debugOut("\nLENG="), XBD_debugOutHex32Bit(sw_flashleng);

		XBD_readPage((sw_flashaddr & PAGE_ALIGN_MASK), sw_flashbuffer);
		sw_flash_fbidx = (sw_flashaddr & ~PAGE_ALIGN_MASK);
		xbd_state = flash;
		sw_flashseqn = 0;
		//sw_flash_fbidx = 0;
		sw_flashprocessed = 0;

		//prepare 'OK' response to XBH
		XBD_loadStringFromConstDataArea((char *)buf, XBDpfo);
	} else {
		XBD_debugOut("Rec'd W-R-O-N-G request to flash:");
		XBD_debugOut("\nADDR="), XBD_debugOutHex32Bit(sw_flashaddr);
		XBD_debugOut("\nLENG="), XBD_debugOutHex32Bit(sw_flashleng);
		//prepare 'FAIL' response to XBH
		XBD_loadStringFromConstDataArea((char *)buf, XBDpff);
	}

//	XBD_debugOut("\n");
//	XBD_debugOut((char *)buf);
//	XBD_debugOut("\n");
	strcpy((char *)XBD_response, (char *)buf);
	realTXlen=XBD_COMMAND_LEN+CRC16SIZE;
	return;
}

void XBD_BL_HandleFlashDownloadRequest(uint8_t len, uint8_t *data) {
	if ((sw_flashseqn == NTOHL(*((uint32_t*) (data + XBD_COMMAND_LEN)))) && //sequence number correct
			(xbd_state == flash)) {
		//put flash programming call here
//		XBD_debugOut("Rec'd correct flash download:");
//		XBD_debugOut("\nSEQN="), XBD_debugOutHex32Bit(sw_flashseqn);
//		XBD_debugOut("\nADDR="), XBD_debugOutHex32Bit(sw_flashaddr);
//		XBD_debugOut("\nLENG="), XBD_debugOutHex32Bit(sw_flashleng);

		if (PAGESIZE >= (sw_flash_fbidx + len - XBD_COMMAND_LEN - SEQNSIZE)) {
			uint8_t *p_src = (void*) (data+XBD_COMMAND_LEN + SEQNSIZE);
			uint8_t cpylen = len - XBD_COMMAND_LEN - SEQNSIZE;
			memcpy(&sw_flashbuffer[sw_flash_fbidx], p_src,cpylen );
			sw_flash_fbidx += len - XBD_COMMAND_LEN - SEQNSIZE;
			sw_flashleng -= len - XBD_COMMAND_LEN - SEQNSIZE;
			++sw_flashseqn;

			if ((PAGESIZE == sw_flash_fbidx) || (0 == sw_flashleng)) {
				//boot_program_page(sw_flashaddr, sw_flashbuffer);
				XBD_programPage((sw_flashaddr & PAGE_ALIGN_MASK), sw_flashbuffer);
				sw_flash_fbidx = 0;
			}

			if (0 == sw_flashleng) {
				xbd_state = idle;
			}

			//prepare 'OK' response to XBH
			XBD_loadStringFromConstDataArea((char *)buf, XBDpfo);
		} else {
			// TODO: downloads von >1 page unterstuetzen
			XBD_debugOut("\nDownload > 1 Page not yet supported.");
			xbd_state = idle;
			XBD_loadStringFromConstDataArea((char *)buf, XBDpff);
		}

	} else {
		XBD_debugOut("Rec'd W-R-O-N-G flash download:");
		XBD_debugOut("\nADDR="), XBD_debugOutHex32Bit(sw_flashaddr);
		XBD_debugOut("\nLENG="), XBD_debugOutHex32Bit(sw_flashleng);
		XBD_debugOut("\nEXPECTED SQ="), XBD_debugOutHex32Bit(sw_flashseqn);
		XBD_debugOut("\nRec'd SQ="), XBD_debugOutHex32Bit(NTOHL(*((uint32_t*) (data + XBD_COMMAND_LEN))));
                //prepare 'FAIL' response to XBH
		XBD_loadStringFromConstDataArea((char *)buf, XBDpff);
	}

//	XBD_debugOut("\n");
//	XBD_debugOut((char *)buf);
//	XBD_debugOut("\n");
	strcpy((char *)XBD_response, (char *)buf);
	realTXlen=XBD_COMMAND_LEN+CRC16SIZE;
	return;
}

void XBD_BL_HandleStartApplicationRequest(){
	XBD_switchToApplication();
}

void XBD_BL_HandleVersionInformationRequest(){
	XBD_loadStringFromConstDataArea((char *)XBD_response, XBDblo);
	realTXlen=XBD_COMMAND_LEN+CRC16SIZE;
}

void XBD_BL_HandleTimingCalibrationRequest(){
	uint32_t cycles_elapsed = XBD_busyLoopWithTiming(DEVICE_SPECIFIC_SANE_TC_VALUE);

	XBD_loadStringFromConstDataArea((char *)XBD_response, XBDtco);
	*(uint32_t *)&XBD_response[XBD_COMMAND_LEN]=HTONL(cycles_elapsed);
		realTXlen=XBD_COMMAND_LEN+TIMESIZE+CRC16SIZE;
}

void XBD_BL_HandleTargetRevisionRequest(){
	uint8_t i;
	uint8_t gitRevLen = strlen(XBD_Rev);

	XBD_loadStringFromConstDataArea((char *)XBD_response, XBDtro);
	
	// Report git Rev in 40 digits length
	for(i=0;i<40-gitRevLen;++i)
	{
		XBD_response[XBD_COMMAND_LEN+i]='0';
	}
	XBD_loadStringFromConstDataArea((char *)&XBD_response[XBD_COMMAND_LEN+i], XBD_Rev);
	realTXlen=XBD_COMMAND_LEN+gitRevLen+CRC16SIZE;
}

void FRW_msgRecHand(uint8_t len, uint8_t* data) {

	uint8_t dataLen=len-CRC16SIZE;
	//output length of received block
	#ifdef XBX_DEBUG_BL
	XBD_debugOut("Rec'd len=");
	XBD_debugOutHexByte(len);
	XBD_debugOutChar('.');
	XBD_debugOut("\n");
	#endif

	//check crc and disregard block if wrong

	rx_crc = UNPACK_CRC(&data[len - CRC16SIZE]);
	crc = crc16buffer(data, len - CRC16SIZE);
	if (rx_crc != crc) {
		XBD_BL_DisregardBlock(len, data);
		return;
	}

	// l 
	// o opback
	// r equest
	XBD_loadStringFromConstDataArea((char *)buf, XBDlor);
	if (0 == strncmp((char *)buf, (char*) data, XBD_COMMAND_LEN)) {
		XBD_BL_HandleLOopbackRequest(dataLen, data);
		return;
	}


	// p rogram
	// f lash
	// r equest
	XBD_loadStringFromConstDataArea((char *)buf, XBDpfr);
	if (0 == strncmp((char *)buf, (char*) data, XBD_COMMAND_LEN)) {
		XBD_BL_HandleProgramFlashRequest(dataLen, data);
		return;
	}

	// f lash
	// d ownload
	// r equest
	XBD_loadStringFromConstDataArea((char *)buf, XBDfdr);
	if (0 == strncmp((char *)buf, (char*) data, XBD_COMMAND_LEN)) {
		XBD_BL_HandleFlashDownloadRequest(dataLen, data);
		return;
	}

	// s tart
	// a application
	// r equest
	XBD_loadStringFromConstDataArea((char *)buf, XBDsar);
	if (0 == strncmp((char *)buf, (char*) data, XBD_COMMAND_LEN)) {
		XBD_BL_HandleStartApplicationRequest();
		return;
	}

	// v ersion
	// i nformation
	// r equest
	XBD_loadStringFromConstDataArea((char *)buf, XBDvir);
	if (0 == strncmp((char *)buf, (char*) data, XBD_COMMAND_LEN)) {
		XBD_BL_HandleVersionInformationRequest();
		return;
	}


	// t iming
	// c alibration
	// r equest
	XBD_loadStringFromConstDataArea((char *)buf, XBDtcr);
	if (0 == strncmp((char *)buf, (char*) data, XBD_COMMAND_LEN)) {
		XBD_BL_HandleTimingCalibrationRequest();
		return;
	}

	// t arget
	// r evision
	// r equest
	XBD_loadStringFromConstDataArea((char *)buf, XBDtrr);
	if (0 == strncmp((char *)buf, (char*) data, XBD_COMMAND_LEN)) {
		XBD_BL_HandleTargetRevisionRequest();
		return;
	}

	//no known command recognized
	XBD_loadStringFromConstDataArea((char *)XBD_response, XBDunk);
	#ifdef XBX_DEBUG_BL
	XBD_debugOut((char *)XBD_response);
	XBD_debugOut("\n");
	#endif
}

uint8_t FRW_msgTraHand(uint8_t maxlen, uint8_t* data) {
 	if(maxlen <= CRC16SIZE) {
        	XBD_debugOut("MsgTraHand: Maxlen too small: ");
        	XBD_debugOutHexByte(maxlen);
                XBD_debugOut("\n");
	}
                                        
	if (maxlen > XBD_ANSWER_MAXLEN)
	{
		//XBD_debugOut("MsgTraHand: Maxlen too large: ");
		//XBD_debugOutHexByte(maxlen);
    //XBD_debugOut("\n");
		maxlen = XBD_ANSWER_MAXLEN;
	}

	memcpy((char*) data, (char *)XBD_response, maxlen-CRC16SIZE);
	crc = crc16buffer(data, realTXlen-CRC16SIZE);                
	uint8_t *target=data+realTXlen-CRC16SIZE;             
	PACK_CRC(crc,target);
	//XBD_debugOutBuffer("FRW_msgTraHand", data, maxlen);
	return maxlen;
}


int main(void)
{  

	XBD_init();

	XBD_debugOut("XBD BL "XBX_REVISION" started\r\n");

	while(1)
	{
		XBD_serveCommunication();

	}

}

