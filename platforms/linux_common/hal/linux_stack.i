#define STACK_CANARY (inv_sc?0x3A:0xC5)
#define STACK_SIZE_TO_PAINT (PLATFORM_STACK_SIZE-1000)
uint8_t *p_stack = NULL;
uint8_t *p = NULL;
uint8_t inv_sc=0;
uint32_t stackCounter1 = 0;
uint32_t stackCounter2 = 0;

void XBD_paintStack(void)
{
  /* initialise stack measurement */
  inv_sc=!inv_sc;
  uint8_t arr [STACK_SIZE_TO_PAINT];
  {
    p = p_stack = &arr[0];
    while(p < &arr[STACK_SIZE_TO_PAINT])
    {
      *p = STACK_CANARY;
      ++p;
    }
  }
}

uint32_t XBD_countStack(void)
{
  /* return stack measurement result */
  stackCounter1 = stackCounter2 = 0;
  {
    p = p_stack;
    while( p < p_stack+STACK_SIZE_TO_PAINT && *p  == STACK_CANARY )
    {
         ++p;
         ++stackCounter1;
    }
    p = p_stack+STACK_SIZE_TO_PAINT-1;
    while(*p  == STACK_CANARY && p > p_stack)
    {
         --p;
         ++stackCounter2;
    }
    return ((stackCounter1 > stackCounter2) ? stackCounter1 : stackCounter2);
  }  
}
