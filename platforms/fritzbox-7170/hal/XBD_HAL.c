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
#include <stdlib.h>
#include "XBD_debug.h"

/* uploaded binary */
#define BINARY_FILE "/var/xbdprog.bin"

int gpioFD;

#include <linux_xbd.i>

#define IP_COMM



#ifdef IP_COMM
  #include <ipcomm.i> 
#else

  /* communication to xbh */
  #define SERIAL_DEVICE "/dev/ttyS0"
  int serialFD;


  int uart_init() {
    struct termios T_new;
    /*open tty port*/
    serialFD = open(SERIAL_DEVICE, O_RDWR);
    if (serialFD == -1) {
      XBD_debugOut("open of serial port failed, errno: ");
      XBD_debugOutHex32Bit(errno);
      XBD_debugOut("\n");
      return 0;
    }

    /*termios functions use to control asynchronous communications ports*/
    if (tcgetattr(serialFD, &T_new) != 0) {	/*fetch tty state*/
      XBD_debugOut("tcgetattr failed. errno: ");
      XBD_debugOutHex32Bit(errno);
      XBD_debugOut("\n");
      close(serialFD);
      return 0; 
    }

    /*set 	115200bps, no parity, no flow control, 
    ignore modem status lines, 
    hang up on last close, 
    and disable other flags*/
    T_new.c_cflag = (B115200 | CS8 | CREAD | CLOCAL | HUPCL); 	
    T_new.c_oflag = 0;
    T_new.c_iflag = 0;
    T_new.c_lflag = 0;
    if (tcsetattr(serialFD, TCSANOW, &T_new) != 0) {
      printf("tcsetattr failed. errno: ");
      XBD_debugOutHex32Bit(errno);
      XBD_debugOut("\n");
      close(serialFD);
      return 0; 
    }
    return 1;
  }

  void uart_close()
  {
    close(serialFD);
  }

  unsigned char readByte()
  {
    unsigned char byte;
    while(!read(serialFD,&byte,1));
    return byte;
  }

  void writeByte(unsigned char byte)
  {
    while(!write(serialFD,&byte,1));
  }
#endif

void gpio_init()
{
  gpioFD=open("/dev/new_led",O_RDWR);
  if(gpioFD==-1) {
    XBD_debugOut("Error opening /dev/new_led");
  }
  XBD_debugOut("GPIO FD: ");
  XBD_debugOutHex32Bit(gpioFD);
  XBD_debugOut("\n");
  XBD_sendExecutionCompleteSignal();
}

void gpio_close()
{
  close(gpioFD);
}


void XBD_init()
{
	linuxXBD_init();

	#ifdef IP_COMM
	  ipcomm_init();
	#else
	  uart_init();
	  writeByte('A');
	#endif

	gpio_init();

	#ifdef IP_COMM
		XBD_debugOut("START Fritzbox 7170 IPCOMM HAL\r\n");
	#else
		XBD_debugOut("START Fritzbox 7170 UART HAL\r\n");
	#endif


	#ifdef I_AM_OS_BASED
		XBD_debugOut("I_AM_OS_BASED! Sleep()ing before work!\r\n");
	#endif		
	XBD_debugOut("\r\n");		 
}


inline void XBD_sendExecutionStartSignal()
{
  char *on="SET info,0=1\n";
  write(gpioFD,on,13);
  fsync(gpioFD);
}


inline void XBD_sendExecutionCompleteSignal()
{
  char *off="SET info,0=0\n";
  write(gpioFD,off,13);
  fsync(gpioFD);
}


#ifndef IP_COMM
#include <XBD_streamcomm.i>
#endif


/* close fds and launch application */
void XBD_switchToApplication()
{
  gpio_close();
  #ifdef IP_COMM
    ipcomm_close();
  #else
    uart_close();
  #endif

  linuxXBD_switchToApplication();

  #ifdef IP_COMM
    ipcomm_init();
  #else
    uart_init();
  #endif
  gpio_init();
}



void XBD_switchToBootLoader()
{
  gpio_close();
  #ifdef IP_COMM
    ipcomm_close();
  #else
    uart_close();
  #endif
  
  linuxXBD_switchToBootLoader();

  binary_close();
  debug_close();
  exit(0);
}


#include <busyloop_gettimeofday.i>
#include <linux_stack.i>
