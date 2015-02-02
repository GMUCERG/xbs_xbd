#include <XBD_HAL.h>
#include <XBD_FRW.h>
#include <string.h>
#include <stdio.h>
#include <termios.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <signal.h>
#include <XBD_DeviceDependent.h>       
#include <XBD_debug.h>

/* uploaded binary */
#define BINARY_FILE "/tmp/xbdprog.bin"

/* log file */
#define DEBUG_FILE "/tmp/xbd.log"

#include <ipcomm.i>
#include <linux_xbd.i>


void gpio_init()
{
}

void gpio_close()
{
}


void XBD_init()
{
        linuxXBD_init();
        ipcomm_init();
        gpio_init();
	XBD_debugOut("START LINUX PC HAL\r\n");
	XBD_debugOut("\r\n");		 
}


inline void XBD_sendExecutionStartSignal()
{
  XBD_debugOut("Ex start\n");
}


inline void XBD_sendExecutionCompleteSignal()
{
  XBD_debugOut("Ex stop\n");
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
