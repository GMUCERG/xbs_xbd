#include <XBD_util.h>

// nibble to ASCII
inline char ntoa(uint8_t n)
{
	if(n<=9)
		return n+'0';
	else
		return n+'A'-10;
}


char * u32toHexString(uint32_t ui32, char *s)
{
	uint8_t i;

	for(i=0;i<8;++i)
	{
		s[i]=ntoa( (ui32>>(28-(4*i))&0xf) );
	}

	return s;
}
