/**************************************************************************/
/*! 
    @file     iap.c
    Source:   http://knowledgebase.nxp.com/showthread.php?t=1594
*/
/**************************************************************************/
#include "iap.h"
 
IAP_return_t iap_return;
 
#define IAP_ADDRESS 0x1FFF1FF1
uint32_t param_table[5];
 
/**************************************************************************/
/*! 
    Sends the IAP command and stores the result
*/
/**************************************************************************/
void iap_entry(uint32_t param_tab[], uint32_t result_tab[])
{
  void (*iap)(uint32_t[], uint32_t[]);
  iap = (void (*)(uint32_t[], uint32_t[]))IAP_ADDRESS;
  iap(param_tab,result_tab);
}

IAP_return_t iapEraseSector(uint32_t FlashSector)
{
	param_table[0] = IAP_CMD_ERASESECTORS;
	param_table[1] = FlashSector;
	param_table[2] = FlashSector;
	param_table[3] = CFG_CPU_CCLK / 1000UL;	/* Core clock frequency in kHz */

	iap_entry(param_table, (uint32_t*)(&iap_return));
	return iap_return;
}

IAP_return_t iapCopyRAMToFlash(uint32_t u32DstAddr, uint32_t u32SrcAddr, uint32_t u32Len)
{
	param_table[0] = IAP_CMD_COPYRAMTOFLASH;
	param_table[1] = u32DstAddr;
	param_table[2] = u32SrcAddr;
	param_table[3] = u32Len;
	param_table[4] = CFG_CPU_CCLK / 1000UL;	/* Core clock frequency in kHz */

	iap_entry(param_table, (uint32_t*)(&iap_return));
	return iap_return;
}

IAP_return_t iapPrepareSector(uint32_t FlashSector)
{	
	param_table[0] = IAP_CMD_PREPARESECTORFORWRITE;
	param_table[1] = FlashSector;
	param_table[2] = FlashSector;

	iap_entry(param_table, (uint32_t*)(&iap_return));
	return iap_return;
}

	
/**************************************************************************/
/*! 
    Returns the CPU's unique 128-bit serial number (4 words long)

    @section Example

    @code
    #include "core/iap/iap.h"

    IAP_return_t iap_return;
    iap_return = iapReadSerialNumber();

    if (iap_return.ReturnCode == 0)
    {
      printf("Serial Number: %08X %08X %08X %08X %s", 
              iap_return.Result[0],
              iap_return.Result[1],
              iap_return.Result[2],
              iap_return.Result[3], 
              CFG_PRINTF_NEWLINE);
    }
    @endcode
*/
/**************************************************************************/
IAP_return_t iapReadSerialNumber(void)
{
  // ToDo: Why does IAP sometime cause the application to halt when read???
  param_table[0] = IAP_CMD_READUID;
  iap_entry(param_table,(uint32_t*)(&iap_return));
  return iap_return;
}

