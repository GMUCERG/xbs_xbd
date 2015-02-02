int debugFD = 0;
int binaryFD = 0;

void debug_init()
{
/*  debugFD=open(DEBUG_FILE, O_RDWR | O_APPEND | O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP );
  if(debugFD==-1) {
    XBD_debugOut("Error opening debug dest");
  }
*/
}

void debug_close()
{
  /* close(debugFD);*/
}


void XBD_debugOut(char *message)
{
  write(STDOUT_FILENO,message,strlen(message));
}


void binary_init()
{
  binaryFD = open(BINARY_FILE , O_RDWR | O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP );
  if(binaryFD==-1)
  {
    XBD_debugOut("Error opening binary dest\n");
    XBD_debugOut(strerror(errno));
    XBD_debugOut("\n");
  }
}


void binary_close()
{
  if (binaryFD > 0)
  {
    close(binaryFD);
    binaryFD = 0;
  }
}

void linuxXBD_switchToApplication()
{
  XBD_debugOut("\n");
  binary_close();
  debug_close();
  chmod(BINARY_FILE, S_IXUSR | S_IRUSR);
  if (-1 == system(BINARY_FILE)) {
	XBD_debugOut("system() failed");
        XBD_debugOut(strerror(errno));
        XBD_debugOut("\n");
  }
  debug_init();
  binary_init();
}

void linuxXBD_switchToBootLoader()
{
  debug_close();
  exit(0);
}

void seekBinary(uint32_t pos)
{
  int32_t currentPos;
  unsigned char zero=0;
  int diff;

  currentPos=lseek(binaryFD,pos,SEEK_SET);
  if(currentPos < pos) {
    /* this should not happen as to the man page of lseek */
    XBD_debugOut("Resizing file");
    diff=pos-currentPos;
    while(diff >0) {
      write(binaryFD,&zero,1);   
      diff--;
    }
  }
}

void XBD_readPage(uint32_t pageStartAddress, uint8_t * buf)
{
  /** fill if zeroes if needed */
  seekBinary(pageStartAddress+PAGESIZE);
  seekBinary(pageStartAddress);
  read(binaryFD,buf,PAGESIZE); 	
}

void XBD_programPage(uint32_t pageStartAddress, uint8_t * buf)
{
  XBD_debugOut(".");
  seekBinary(pageStartAddress);
  write(binaryFD,buf,PAGESIZE);
}

void linuxXBD_init()
{
  debug_init();
  #ifdef BOOTLOADER
  binary_init();
  #endif
}

void XBD_loadStringFromConstDataArea(char *dst, const char *src)
{
  strcpy(dst,src);
}

void alarmhandler(int sig)
{
  if (sig == SIGALRM)
  {
    XBD_debugOut("Caught SIGALRM, exiting\n");
    debug_close();
    #ifdef IP_COMM
    ipcomm_close();
    #endif
    exit(1);
  } else {
    XBD_debugOut("Caught SIGNAL ");
    XBD_debugOutHex32Bit(sig);
    XBD_debugOut("\n");
  }
}

void XBD_startWatchDog(uint32_t seconds) {
  #ifdef XBD_WATCHDOG_DEBUG
  XBD_debugOut("setup a watchdog timer with ");
  XBD_debugOutHex32Bit(seconds);
  XBD_debugOut(" seconds\n");
  #endif
  signal(SIGALRM, alarmhandler);
  alarm(seconds);
}

void XBD_stopWatchDog() {
  uint32_t remaining;
  signal(SIGALRM, SIG_IGN);
  remaining = alarm(0);
  #ifdef XBD_WATCHDOG_DEBUG
  XBD_debugOut("stopped the watchdog timer with ");
  XBD_debugOutHex32Bit(remaining);
  XBD_debugOut(" seconds remaining\n");
  #endif
} 
