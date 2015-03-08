#include <stdint.h>

#include <XBD_commands.h>
#include <XBD_debug.h>

#include "XBD_multipacket.h"

#include <string.h>

uint8_t realTXlen;

uint32_t xbd_genmp_seqn,xbd_genmp_dataleft,xbd_genmp_datanext;
uint32_t xbd_recmp_seqn,xbd_recmp_dataleft,xbd_recmp_datanext,xbd_recmp_type,xbd_recmp_addr;

uint32_t XBD_genSucessiveMultiPacket(const uint8_t* srcdata, uint8_t* dstbuf, uint32_t dstlenmax, const char  *code)
{
	uint32_t offset=0;
	uint32_t cpylen;

	if(0 == xbd_genmp_dataleft)
		return 0;

	memset(dstbuf,0x77,dstlenmax);
	
		

	XBD_loadStringFromConstDataArea((char *)dstbuf, code);
	offset+=XBD_COMMAND_LEN;

	//XBD_debugOutBuffer("smp1:",dstbuf, XBD_ANSWERLENG_MAX+2);

	*((uint32_t*) (dstbuf + offset)) = HTONL(xbd_genmp_seqn);
	xbd_genmp_seqn++;
	offset+=SEQNSIZE;

	//XBD_debugOutBuffer("smp2:",dstbuf, XBD_ANSWERLENG_MAX+2);

	cpylen=(dstlenmax-offset);	//&(~3);	//align 32bit
	if(cpylen > xbd_genmp_dataleft)
		cpylen=xbd_genmp_dataleft;

	memcpy((dstbuf+offset),(srcdata+xbd_genmp_datanext),cpylen);


	xbd_genmp_dataleft-=cpylen;
	xbd_genmp_datanext+=cpylen;
	offset+=cpylen;



//	XBD_debugOutBuffer("offset:",&offset, 2);


//	XBD_debugOutBuffer("smp3:",dstbuf, XBD_ANSWERLENG_MAX+2);


//	XBD_debugOutBuffer("smp4:",dstbuf, XBD_ANSWERLENG_MAX+2);
	realTXlen=XBD_ANSWERLENG_MAX;
	return offset;
}

uint32_t XBD_genInitialMultiPacket(const uint8_t* srcdata, uint32_t srclen, uint8_t* dstbuf,const uint8_t *code, uint32_t type, uint32_t addr)
{
	uint32_t offset=0;

	xbd_genmp_seqn=0;
	xbd_genmp_datanext=0;
	xbd_genmp_dataleft=srclen;

	XBD_loadStringFromConstDataArea((char *) dstbuf, (char *)code);
	offset+=XBD_COMMAND_LEN;


	if(0xffffffff != type)
	{	
		//uint32_t target=dstbuf+offset;
		//XBD_debugOutBuffer("type:",&type, 4);
		//XBD_debugOutBuffer("target:",&target, 4);
	
		
		*((uint32_t*) (dstbuf + offset)) = HTONL(type);
		//XBD_debugOutBuffer("type@target:",dstbuf+offset, 4);
		offset+=TYPESIZE;
	}

	if(0xffffffff != addr)
	{
		*((uint32_t*) (dstbuf + offset)) = HTONL(addr);
		offset+=ADDRSIZE;
	}


	
	*((uint32_t*) (dstbuf + offset)) = HTONL(srclen);
	//XBD_debugOutBuffer("srclen:",&srclen, 4);
	//XBD_debugOutBuffer("srclen@target:",dstbuf+offset, 4);
	offset+=LENGSIZE;
#ifdef DEBUG
	XBD_DEBUG("\noffset="); XBD_DEBUG_32B(offset);
    XBD_DEBUG("\n");
#endif
	realTXlen=XBD_ANSWERLENG_MAX;
	return offset;
}

uint8_t XBD_recSucessiveMultiPacket(const uint8_t* recdata, uint32_t reclen, uint8_t* dstbuf, uint32_t dstlenmax, const char *code)
{
	uint32_t offset=0;
	uint32_t cpylen;
	char strtmp[XBD_COMMAND_LEN+1];

	if(0 == xbd_recmp_dataleft)
		return 0;

	XBD_loadStringFromConstDataArea(strtmp, code);
	if(strncmp(strtmp,(char *) recdata,XBD_COMMAND_LEN))
		return 1;	//wrong command code
	offset+=XBD_COMMAND_LEN;
	if(offset > reclen)
		return 2;	//rec'd packet too shor

	if(xbd_recmp_seqn==NTOHL(*((uint32_t*) (recdata + offset))))
	{
		offset+=SEQNSIZE;
		if(offset > reclen)
			return 2;	//rec'd packet too short
		++xbd_recmp_seqn;
	}
	else
		return 3;	//seqn error


	cpylen=(reclen-offset);	//&(~3);	//align 32bit
	if(cpylen > xbd_recmp_dataleft)
		cpylen=xbd_recmp_dataleft;

	if(xbd_recmp_datanext+cpylen > dstlenmax)
		return 4;	//destination buffer full

	memcpy((dstbuf+xbd_recmp_datanext),(recdata+offset),cpylen);
	xbd_recmp_dataleft-=cpylen;
	xbd_recmp_datanext+=cpylen;
	offset+=cpylen;

	return 0;	//OK
}

uint8_t XBD_recInitialMultiPacket(const uint8_t* recdata, uint32_t reclen, const char *code, uint8_t hastype, uint8_t hasaddr)
{
	uint32_t offset=0;
	char strtmp[XBD_COMMAND_LEN+1];

	xbd_recmp_seqn=0;
	xbd_recmp_datanext=0;

	XBD_loadStringFromConstDataArea(strtmp, code);
	if(strncmp(strtmp,(const char *)recdata,XBD_COMMAND_LEN))
		return 1;	//wrong command code

	offset+=XBD_COMMAND_LEN;
	if(offset > reclen)
		return 2;	//rec'd packet too short

	if(hastype)
	{
		xbd_recmp_type=NTOHL(*((uint32_t*) (recdata + offset)));
		offset+=TYPESIZE;
		if(offset > reclen)
			return 2;	//rec'd packet too short
	}

	if(hasaddr)
	{
		xbd_recmp_addr=NTOHL(*((uint32_t*) (recdata + offset)));
		offset+=ADDRSIZE;
		if(offset > reclen)
			return 2;	//rec'd packet too short
	}

	xbd_recmp_dataleft=NTOHL(*((uint32_t*) (recdata + offset)));
	offset+=LENGSIZE;
	if(offset > reclen)
		return 2;	//rec'd packet too short

	return 0;	//OK
}
