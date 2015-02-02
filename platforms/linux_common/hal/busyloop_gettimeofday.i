#include <sys/time.h>
#include <XBD_debug.h>

double timeOfDay()
{
  double result;
  struct timeval t;
  gettimeofday(&t,(struct timezone *) 0);

  result = t.tv_usec;
  result *= 0.000001;
  result += (double) t.tv_sec;
  return result;
}


/* we can't assume a perf counter */
uint32_t XBD_busyLoopWithTiming(uint32_t approxCycles)
{
  struct timeval tStart;
  struct timeval tEnd;
  double diff;
  uint32_t usecs;   



  gettimeofday(&tStart,(struct timezone *) 0);
  XBD_sendExecutionStartSignal();


  usleep(TC_VALUE_SEC*1000000);
  

  gettimeofday(&tEnd,(struct timezone *) 0);
  XBD_sendExecutionCompleteSignal();

  uint32_t seconds=tEnd.tv_sec-tStart.tv_sec;
  
  if(tEnd.tv_usec > tStart.tv_usec) {
    usecs=tEnd.tv_usec-tStart.tv_usec;
  } else {   
    seconds--;
    usecs=1000000+tEnd.tv_usec-tStart.tv_usec;
  } 

  XBD_debugOut("Result of tc\n");
  XBD_debugOutHex32Bit(seconds);
  XBD_debugOut("s ");
  XBD_debugOutHex32Bit(usecs);
  XBD_debugOut("us\n");

   
  diff=(double) seconds+usecs/1000000.0;
  uint32_t cycles=diff*DEVICE_SPECIFIC_SANE_TC_VALUE/TC_VALUE_SEC;
  XBD_debugOut("cycles: ");
  XBD_debugOutHex32Bit(cycles);
  XBD_debugOut("\n");

  return(cycles);
}

void XBD_delayCycles(uint32_t approxCycles)
{
  struct timeval tStart;
  struct timeval tNow;
  
  gettimeofday(&tStart,(struct timezone *) 0);

 
  uint32_t approxUsecs= (approxCycles*1000) / (uint32_t)((DEVICE_SPECIFIC_SANE_TC_VALUE/1000)/TC_VALUE_SEC) ;

  uint32_t usecsElapsed=0;
  while(usecsElapsed < approxUsecs) {
      gettimeofday(&tNow,(struct timezone *) 0);
	  
	  usecsElapsed=(tNow.tv_sec-tStart.tv_sec)*1000000 + tNow.tv_usec-tStart.tv_usec;
  }
}
