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
#include <XBD_debug.h>

/* uploaded binary */
#define BINARY_FILE "/tmp/xbdprog.bin"

/* log file */
#define DEBUG_FILE "/tmp/xbd.log"

#define IP_COMM

#ifdef IP_COMM
  #include <ipcomm.i>

#else // serial communication

  /* communication to xbh */
  #define SERIAL_DEVICE "/dev/ttyS0"

  /*matrix io defines*/
  #define UC500_GET_UART_TYPE	0xe001  
  #define UC500_SET_UART_TYPE	0xe002

  #define UC500_UART_TYPE_232	232
  #define UC500_UART_TYPE_422	422
  #define UC500_UART_TYPE_485	485

  int serialFD;

  int uart_init() {
    int interface;
    struct termios T_new;
    /*open tty port*/
    serialFD = open(SERIAL_DEVICE, O_RDWR);
    if (serialFD == -1) {
      XBD_debugOut("open of serial port failed, errno: ");
      XBD_debugOutHex32Bit(errno);
      XBD_debugOut("\n");
      return 0;
    }

    /*set serial interface: RS-232*/

    interface = UC500_UART_TYPE_232;
    if(ioctl(serialFD, UC500_SET_UART_TYPE, &interface) != 0) {
      XBD_debugOut("set UART type: failed, errno: ");
      XBD_debugOutHex32Bit(errno);
      XBD_debugOut("\n");
      return 0;
      close(serialFD);
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

int gpio_pin=8;
#define GPIO_PIN &gpio_pin


/* /dev/gpio, GPIO IOCTL Commands */
#define GPIO_IOCTL_COUNT	0
#define GPIO_IOCTL_DIPSW	1
#define GPIO_IOCTL_GET		2
#define	GPIO_IOCTL_SET		3
#define	GPIO_IOCTL_CLEAR	4
#define GPIO_IOCTL_OUTPUT	5
#define GPIO_IOCTL_INPUT	6
#define GPIO_IOCTL_MODE	        7

int gpioFD;

#include <linux_xbd.i>


void gpio_init()
{
  int result=0;


  gpioFD=open("/dev/gpio",O_RDWR);
  if(gpioFD==-1) {
    XBD_debugOut("Error opening /dev/gpio");
  }
  XBD_debugOut("GPIO FD: ");
  XBD_debugOutHex32Bit(result);
  XBD_debugOut("\n");
  
  
  result = ioctl(gpioFD, GPIO_IOCTL_OUTPUT, GPIO_PIN);
  XBD_debugOut("Result of dir ioctl: ");
  XBD_debugOutHex32Bit(result);
  XBD_debugOut("\n");
  
  result=ioctl(gpioFD, GPIO_IOCTL_SET, GPIO_PIN); 
  XBD_debugOut("Result of set ioctl: ");
  XBD_debugOutHex32Bit(result);
  XBD_debugOut("\n");
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
	XBD_debugOut("START M501 HAL\r\n");
	XBD_debugOut("\r\n");		 
}


inline void XBD_sendExecutionStartSignal()
{
  ioctl(gpioFD, GPIO_IOCTL_CLEAR, GPIO_PIN);
}


inline void XBD_sendExecutionCompleteSignal()
{
  ioctl(gpioFD, GPIO_IOCTL_SET, GPIO_PIN); 
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
