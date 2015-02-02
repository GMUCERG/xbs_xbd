#include <XBD_HAL.h>
#include <XBD_FRW.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <XBD_DeviceDependent.h>       
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "XBD_debug.h"

/* uploaded binary */
#define BINARY_FILE "/tmp/xbdprog.bin"
#define IP_COMM

int gpioFD;
#include <ipcomm.i>
#include <linux_xbd.i>

inline void XBD_sendExecutionStartSignal()
{
  char off='0';
  write(gpioFD,&off,1);
  fsync(gpioFD);
}


inline void XBD_sendExecutionCompleteSignal()
{
  char on='1';
  write(gpioFD,&on,1);
  fsync(gpioFD);
}


void gpio_init()
{
  gpioFD=open("/sys/class/leds/nslu2:green:disk-1/brightness",O_RDWR);
  if(gpioFD==-1) {
    XBD_debugOut("Error opening /sys/class/leds/nslu2:green:disk-1/brightness");
  }
  XBD_debugOut("GPIO FD: ");
  XBD_debugOutHex32Bit(gpioFD);
  XBD_debugOut("\n");
  /* shed some light in the darkness */
  XBD_sendExecutionCompleteSignal();
}

void gpio_close()
{
  close(gpioFD);
}

void XBD_init()
{
  linuxXBD_init();
  ipcomm_init();
  gpio_init();
  XBD_debugOut("START NSLU2 OpenWrt HAL\r\n");
  XBD_debugOut("\r\n");		 
}


/* close fds and launch application */
void XBD_switchToApplication()
{
  gpio_close();
  ipcomm_close();

  linuxXBD_switchToApplication();

  ipcomm_init();
  gpio_init();
}



void XBD_switchToBootLoader()
{
  gpio_close();
  ipcomm_close();
  
  linuxXBD_switchToBootLoader();

  binary_close();
  debug_close();
  exit(0);
}


#include <busyloop_gettimeofday.i>
#include <linux_stack.i>
