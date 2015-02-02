#include "XBD_HAL.h"
#include "XBD_debug.h"
#include "XBD_util.h"

void XBD_debugOutHexByte(uint8_t byte)
{
	char buf[3];
	buf[0]=ntoa(byte/16);
	buf[1]=ntoa(byte%16);
	buf[2]=0;
	XBD_debugOut(buf);
}

void XBD_debugOutChar(uint8_t c)
{
	char buf[2];
	buf[0]=c;
	buf[1]=0;
	XBD_debugOut(buf);
}

void XBD_debugOutHex32Bit(uint32_t toOutput)
{
	char buf[9];
	u32toHexString(toOutput,buf);
	buf[8]=0;
	XBD_debugOut(buf);

}

void XBD_debugOutBuffer(char *name, uint8_t *buf, uint16_t len)
{
	uint16_t ctr;
	unsigned char c;

	XBD_debugOut("\n BUF => ");
	XBD_debugOut(name);
	XBD_debugOut(", 0x");
	XBD_debugOutHex32Bit(len);
	XBD_debugOut(" bytes :");
	
	for (ctr = 0; ctr < len; ++ctr) {
		if (0 == ctr % 16)
			XBD_debugOutChar('\n');
		XBD_debugOutHexByte(buf[ctr]);
	}
	XBD_debugOut("\n ASCII ");	
	for (ctr = 0; ctr < len; ++ctr) {
		if (0 == ctr % 16)
			XBD_debugOutChar('\n');
		c=buf[ctr];
		if(c >= 32 && c < 128) XBD_debugOutChar(buf[ctr]);
		else XBD_debugOutChar('?');
		XBD_debugOutChar(' ');
	}
	XBD_debugOut("\n");	
}
