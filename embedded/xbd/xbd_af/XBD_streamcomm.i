/*
* XBD_streamcomm.i
* Provides emulation of TWI (I²C) over stream oriented channel (e.g. UART).
* May be included by HALs that want to use it, otherwise unused.
* Requires the HAL to provide
*
* unsigned char readByte();
* and
* void writeByte(unsigned char byte);
*
* (both blocking)
*
* ALSO REQUIRES the HAL to send an 'A'cknowledge byte after starting up
*
*
*/

/** rs232 transmit buffer */
uint32_t aligned_transmitBuffer[256/sizeof(uint32_t)];
uint8_t  *transmitBuffer=(uint8_t *)aligned_transmitBuffer;

void XBD_serveCommunication()
{
  unsigned char byte;
  
  /* scan for preamble, try again later if not found */
  byte=readByte();
  if(byte!=0) {
    //XBD_debugOut("Received not-null-byte while waiting for preamble ");
    XBD_debugOutHexByte(byte);
    XBD_debugOut(" ");
    XBD_debugOutChar(byte);
    XBD_debugOut("\n");
    return;
  }
  byte=readByte();
  if(byte!=0) return;
  byte=readByte();
  /* read away rest of preamble */
  while(byte==0) byte=readByte();


  unsigned char requestType=byte;
  volatile unsigned char size=readByte();
  int i;
  
  if(requestType=='R') {
   // XBD_debugOut("Received read request of size ");
   // XBD_debugOutHexByte(size);
   // XBD_debugOut("\n");
    FRW_msgTraHand(size,transmitBuffer);
   // XBD_debugOutBuffer("Send buffer",transmitBuffer,size);
    for(i=0; i< size;i++) writeByte(transmitBuffer[i]);
  } else if(requestType=='W') {
   // XBD_debugOut("Received write request of size ");
   // XBD_debugOutHexByte(size);
   // XBD_debugOut("\n");
    for(i=0;i<size;i++) {
      transmitBuffer[i]=readByte();
    }
    FRW_msgRecHand(size,transmitBuffer);
	writeByte('A');	//'ACK' receipt to XBH
  }
}
