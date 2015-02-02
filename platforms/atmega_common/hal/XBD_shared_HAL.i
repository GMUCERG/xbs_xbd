#define STACK_CANARY (inv_sc?0x3A:0xC5)
extern uint8_t _end;
extern uint8_t __stack;

void XBD_delayCycles(uint32_t approxCycles)
{
	uint32_t exactCycles=0;	
	uint16_t lastTCNT=0;
	uint16_t overRuns=0;
	
	TCCR1B = (1<<CS10 );	//normal mode, full CLKcpu
	TIMSK1 = 0;	//no interrupts
	TCNT1 = 0;
	
	while(exactCycles < approxCycles)
	{
		if(lastTCNT > TCNT1)
		{
			++overRuns;
		}
		lastTCNT=TCNT1;
		exactCycles = ((uint32_t)overRuns<<16)|TCNT1;
	}
	
	return;
}

uint32_t XBD_busyLoopWithTiming(uint32_t approxCycles)
{
	uint32_t exactCycles=0;	
	uint16_t lastTCNT=0;
	uint16_t overRuns=0;

	TCCR1B = (1<<CS10 );	//normal mode, full CLKcpu
	TIMSK1 = 0;	//no interrupts
	
	TCNT1 = 0;
	XBD_sendExecutionStartSignal();

	while(exactCycles < approxCycles)
	{
		if(lastTCNT > TCNT1)
		{
			++overRuns;
		}
		lastTCNT=TCNT1;
		exactCycles = ((uint32_t)overRuns<<16)|TCNT1;
	}   	

	exactCycles = ((uint32_t)overRuns<<16);
	exactCycles |= TCNT1;
	XBD_sendExecutionCompleteSignal();

	return exactCycles;
}

void XBD_loadStringFromConstDataArea( char *dst, const char *src  ) {
#ifndef FLASHGT64K
	strcpy_P((char *)dst,(const char *)src);
#else
	#ifndef UPPER64K
		while( (*dst++=pgm_read_byte_far( ((uint16_t)src++) )) );
	#else
		while( (*dst++=pgm_read_byte_far( (0x10000l)|((uint32_t)((uint16_t)src++)))) );
	#endif
#endif	
}

void XBD_switchToApplication()
{
  void (*reboot)( void ) = 0x0000;
  reboot();
}


void XBD_switchToBootLoader()
{
  wdt_enable(WDTO_15MS);
  while(1);
}


void XBD_readPage( uint32_t pageStartAddress, uint8_t * buf )
{
#ifndef FLASHGT64K
	u16 startAddress=(u16) pageStartAddress;
	memcpy_P(buf, (const prog_void *)startAddress, 256);
#else
	u16 u;

	for(u=0;u<256;++u)
	{
		*buf++=pgm_read_byte_far(pageStartAddress++);
	}
#endif
	
}

void XBD_programPage( uint32_t pageStartAddress, uint8_t * buf )
{
	uint16_t i;

	eeprom_busy_wait();

	boot_page_erase(pageStartAddress);
	boot_spm_busy_wait(); // Wait until the memory is erased.

	for (i = 0; i < SPM_PAGESIZE; i += 2) {
		// Set up little-endian word.

		uint16_t w = *buf++;
		w += (*buf++) << 8;

		boot_page_fill(pageStartAddress + i, w);
	}

	boot_page_write(pageStartAddress); // Store buffer in flash page.
	boot_spm_busy_wait(); // Wait until the memory is written.

	// Reenable RWW-section again. We need this if we want to jump back
	// to the application after bootloading.

	boot_rww_enable();

}


uint8_t *p_stack = &__stack;
uint8_t *p = &_end;
uint8_t inv_sc=0;

void XBD_paintStack(void)
{
	inv_sc=!inv_sc;

	p = &_end;
 	p_stack = alloca(1);   	
    while(p <= p_stack)
    {
        *p = STACK_CANARY;
        ++p;
    } 
}

uint32_t XBD_countStack(void)
{
    p = &_end;
    register uint16_t c = 0;

    while(*p == STACK_CANARY && p <= p_stack)
    {
        ++p;
        ++c;
    }

    return (uint32_t)c;
} 

void XBD_startWatchDog(uint32_t seconds)
{
  (void)seconds;
}

void XBD_stopWatchDog()
{
}
