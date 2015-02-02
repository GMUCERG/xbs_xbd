#define IPCOMM_PORT 22596

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>

#include <XBD_crc.h>

// #define XBX_DEBUG_TRANSPORT

#ifdef BOOTLOADER
#define IPCOMMDBG "BL ipcomm: "
#else
#define IPCOMMDBG "AP ipcomm: "
#endif

#define XBD_MAGIC_SEQ_NO 	0

#define XBD_IDLE		0
#define XBD_ACK_WAIT		1

#define XBD_ACK_ERROR		0
#define XBD_ACK_SUCCESS		1
#define XBD_ACK_TIMEOUT		2


const uint8_t XBD_WRITE_ACK       = 'A'; // send ack write packets
const uint8_t XBD_IPCOMM_ACK      =  0xAC; // protocol ack byte
const uint8_t XBD_IPCOMM_DATA_HDR = 'D';   // protocol data byte

uint8_t recv_seq_no = 0;
uint8_t prev_seq_no = 0;
uint8_t send_seq_no = 0;
uint8_t wait_seq_no = 0;

uint8_t xbd_ipcomm_state = XBD_IDLE;

unsigned char transmitSize = 0;
int32_t bytes_read = 0;

struct sockaddr_in sender = {0,};
socklen_t sock_len = sizeof(sender);
socklen_t rsock_len = sizeof(sender);

int connFD = 0;

uint32_t alignedBuffer[512/sizeof(uint32_t)+1];
uint8_t* transmitBuffer=((uint8_t*)alignedBuffer)+1;
uint8_t* XBD_hdr = ((uint8_t*)alignedBuffer)+1;


void ipcomm_transmit_data(const uint8_t);

inline uint8_t next_seq_no()
{
  uint8_t r = send_seq_no++;
  if (0==send_seq_no) send_seq_no++;
  return r;
}

inline int udp_write(const uint8_t *buf, uint16_t len)
{
  #ifdef DEVICE_SPECIFIC_IPCOMM_DELAY
    usleep(DEVICE_SPECIFIC_IPCOMM_DELAY);
  #endif
  return sendto(connFD, buf, len, 0, (struct sockaddr *)&sender, sock_len);
}

inline int udp_input_wait(int timeout)
{
  struct timeval tv;
  fd_set rd;
  FD_ZERO(&rd);
  FD_SET(connFD, &rd);
  tv.tv_sec = timeout;
  tv.tv_usec = 0;
  return select(connFD+1, &rd, NULL, NULL, &tv);
}

inline int udp_read(uint8_t *buf, uint16_t len)
{
  rsock_len = sizeof(sender);
  return recvfrom(connFD, buf, len, 0, (struct sockaddr *)&sender, &rsock_len);
}

void ipcomm_init()
{
  struct sockaddr_in sock = {0,};
  connFD = socket(PF_INET, SOCK_DGRAM, 0);
  if(connFD <=0 ) {
    XBD_debugOut(IPCOMMDBG "Unable to open listening fd ");
    XBD_debugOut(strerror(errno));
    XBD_debugOut("\n");
    return;
  }

  long one=1;
  int rv = setsockopt(connFD, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(long));
  if (rv < 0)
  {
    XBD_debugOut(IPCOMMDBG "Unable to set socket options ");
    XBD_debugOut(strerror(errno));
    XBD_debugOut("\n");
    return;
  }

  memset(&sock, 0, sizeof(sock));
  sock.sin_family = AF_INET;
  sock.sin_addr.s_addr = HTONL(INADDR_ANY);
  sock.sin_port = HTONS(IPCOMM_PORT);

  rv = bind( connFD, (struct sockaddr *) &sock, sizeof(sock));
  if (rv < 0)
  {
    XBD_debugOut(IPCOMMDBG "Unable to bind socket to port\n");
    return;
  }

  XBD_debugOut(IPCOMMDBG "connFD: ");
  XBD_debugOutHex32Bit(connFD);
  XBD_debugOut("\n");

  XBD_debugOut(IPCOMMDBG "state: ");
  XBD_debugOutHexByte(xbd_ipcomm_state);
  XBD_debugOut("\n");
#ifndef BOOTLOADER
  {
    char *ip = getenv("XBH_RIP");
    char *port = getenv("XBH_RPORT");
    if (NULL != ip && NULL != port)
    {
      XBD_debugOut(IPCOMMDBG "got environment XBH_RIP=");
      XBD_debugOut(ip);
      XBD_debugOut(" XBH_RPORT=");
      XBD_debugOut(port);
      XBD_debugOut("\n");
      inet_pton(AF_INET, ip, &sender.sin_addr);
      sender.sin_port = HTONS((unsigned short)strtoul(port, NULL, 10));
      // acknowledge the switch to application
      transmitBuffer[3] = XBD_WRITE_ACK;
      transmitSize = 1;
      ipcomm_transmit_data(next_seq_no());
      xbd_ipcomm_state = XBD_ACK_WAIT;
    }
    else
    {
      XBD_debugOut(IPCOMMDBG "environment for XBH address not set\n");
    }
  }
#endif
}

void ipcomm_close()
{
  if(connFD > 0)
  {
    close(connFD);
    connFD = 0;
  }
}

void ipcomm_send_ack()
{
  XBD_hdr[0] = XBD_IPCOMM_ACK;
  XBD_hdr[1] = recv_seq_no;
  if (udp_write(&XBD_hdr[0], 2) != 2)
  {
    XBD_debugOut(IPCOMMDBG "error sending ack for seq_no ");
    XBD_debugOutHexByte(recv_seq_no);
    XBD_debugOut("\n");
    return;
  }
}

int ipcomm_ack_wait()
{
  uint8_t ack_hdr[2];
 
  int ret = udp_input_wait(30);

  if (ret > 0)
  {
    if (udp_read(&ack_hdr[0], sizeof(ack_hdr)) == sizeof(ack_hdr)) 
    {
      if (ack_hdr[0] == XBD_IPCOMM_ACK)
      { 
        if (ack_hdr[1] == wait_seq_no)
        {
          return XBD_ACK_SUCCESS;
	}
        else
        {
          XBD_debugOut(IPCOMMDBG "ack for unknown seq_no received ");
	  XBD_debugOutHexByte(ack_hdr[0]);
          XBD_debugOutHexByte(ack_hdr[1]);
          XBD_debugOut("\n");
          return XBD_ACK_ERROR;
        }
      }
      else if (XBD_IPCOMM_DATA_HDR == ack_hdr[0])
      {
	if (recv_seq_no == ack_hdr[1])
        {
          XBD_debugOut(IPCOMMDBG "waited for ack, already acked packet seq_no received ");
          XBD_debugOutHexByte(ack_hdr[1]);
          XBD_debugOut(" reacking\n");
          ipcomm_send_ack();
          return XBD_ACK_ERROR;
	} else {
          XBD_debugOut(IPCOMMDBG "waited for ack, data packet with seq_no ");
          XBD_debugOutHexByte(ack_hdr[1]);
	  XBD_debugOut(" last_acked ");
          XBD_debugOutHexByte(recv_seq_no);
          XBD_debugOut(" received\n");
          return XBD_ACK_ERROR;
        }	
      }
      else
      {
	XBD_debugOut(IPCOMMDBG "ivalid packet received ");
        XBD_debugOutBuffer("ack_hdr", &ack_hdr[0], 2);
	return XBD_ACK_ERROR;
      }
    }
    else
    {
      XBD_debugOut(IPCOMMDBG "unable to receive 2 bytes ack: ");
      XBD_debugOut(strerror(errno));
      XBD_debugOut("\n");
    }
  }
  else if (ret < 0)
  { 
    XBD_debugOut(IPCOMMDBG "select failed: ");
    XBD_debugOut(strerror(errno));
    XBD_debugOut("\n");
  }
  else
  {
    XBD_debugOut(IPCOMMDBG "timeout while receving ack\n");
  }
  return XBD_ACK_TIMEOUT;
}


void ipcomm_transmit_data(const uint8_t seq)
{
  XBD_hdr[1] = XBD_IPCOMM_DATA_HDR;
  wait_seq_no = XBD_hdr[2] = seq;
  if (udp_write(transmitBuffer+1, transmitSize+2) != (transmitSize+2))
  {
    XBD_debugOut(IPCOMMDBG "failed to transmit data");
    XBD_debugOut(strerror(errno));
    XBD_debugOut("\n");
  }
}


void ipcomm_next_layer()
{
  switch(transmitBuffer[2])
  {
    case 'W':
      #ifdef XBX_DEBUG_TRANSPORT
      XBD_debugOutBuffer("Transmitbuffer", transmitBuffer, bytes_read);
      #endif
      FRW_msgRecHand(bytes_read-3,&transmitBuffer[3]);
      transmitSize = 1;
      transmitBuffer[3] = XBD_WRITE_ACK;
    break;
    case 'R':
      transmitSize = transmitBuffer[3];
      FRW_msgTraHand(transmitSize, &transmitBuffer[3]);
    break;
    default:
      XBD_debugOut(IPCOMMDBG "invalid request ");
      XBD_debugOutHexByte(transmitBuffer[2]);
      XBD_debugOut(" 52 or 57 expected\n");
      XBD_debugOutBuffer("transmitBuffer", transmitBuffer, bytes_read);
    break;
  }	
}

inline uint8_t ipcomm_crc16_check()
{
  if (transmitBuffer[2] == 'W')
  {
    const uint8_t *p_crc = &transmitBuffer[bytes_read-2];
    const uint8_t *p_xbd_buf = &transmitBuffer[3];
    uint16_t len = bytes_read-3-2;
    uint16_t crc = crc16buffer(p_xbd_buf, len);
    uint16_t recvd_crc = UNPACK_CRC(p_crc);
    if (crc == recvd_crc)
    {
	return 1;
    }
    else
    {
	XBD_debugOutBuffer(IPCOMMDBG "transmitBuffer", transmitBuffer, bytes_read);
	XBD_debugOut("CRC check failed: calc'd ");
        XBD_debugOutHexByte(crc >> 8);
        XBD_debugOutHexByte(crc & 0xff);
	XBD_debugOut(" rec'd ");
        XBD_debugOutHexByte(recvd_crc >> 8);
        XBD_debugOutHexByte(recvd_crc & 0xff);
        XBD_debugOut("\n");
	return 0;
    }
  }
  else
  {
    // there's no crc
    return 1;
  }
}


uint8_t ipcomm_receive()
{
  bytes_read = udp_read(transmitBuffer, sizeof(alignedBuffer)-1);
  if (bytes_read <= 0)
  {
    XBD_debugOut(IPCOMMDBG "read error, bytes_read: ");
    XBD_debugOutHex32Bit(bytes_read);
    if (bytes_read != 0)
    {
      XBD_debugOut(" error: ");
      XBD_debugOut(strerror(errno));
    }
    XBD_debugOut("\n");
    return 0;
  }
  else if (bytes_read >= 2)
  {
    if (XBD_hdr[0] != 'D') {
      XBD_debugOut(IPCOMMDBG "invalid header byte rec'd ");
      XBD_debugOutHexByte(XBD_hdr[0]); 
      XBD_debugOut(" 44 expected\n");
      XBD_debugOutBuffer(IPCOMMDBG "transmitBuffer", transmitBuffer, bytes_read);
      return 0;
    }
    else
    {
      return 1;
    }
  } // just ignore anything else
  return 0;
}


void XBD_serveCommunication()
{
  switch(xbd_ipcomm_state) {
    case XBD_IDLE:
      //XBD_debugOut(IPCOMMDBG "state XBD_IDLE\n");
      // wait for requests from XBD
      if (0 != ipcomm_receive())
      {
#ifdef BOOTLOADER
        {
           static int s_env_set = 0;
           if (0 == s_env_set)
           {
              char buffer[41];
              setenv("XBH_RIP", inet_ntop(AF_INET, &sender.sin_addr, buffer, sizeof(buffer)), 1);
              sprintf(buffer,"%d",NTOHS(sender.sin_port));
              setenv("XBH_RPORT", buffer, 1);
              s_env_set = 1;
           }
        }
#endif
 	if (0 == ipcomm_crc16_check()) {
	   break;
        }

        recv_seq_no = XBD_hdr[1];

        if (recv_seq_no == XBD_MAGIC_SEQ_NO || recv_seq_no != prev_seq_no)
        {
          // magic seq no or new seq no rec'd 
          ipcomm_send_ack();
          ipcomm_next_layer();
          ipcomm_transmit_data(next_seq_no());
          prev_seq_no = recv_seq_no;
          xbd_ipcomm_state = XBD_ACK_WAIT;
        } 
        else if (prev_seq_no == recv_seq_no)
        {
          // duplicate
          XBD_debugOut(IPCOMMDBG "duplicate seq_no, retransmitting ack\n");
          ipcomm_send_ack();
        }
      } 
      else
      {
	return;
      }
    break;
    case XBD_ACK_WAIT:
      //XBD_debugOut(IPCOMMDBG "state XBD_ACK_WAIT\n");
      while(1) {
        int ack_res = ipcomm_ack_wait();
        if (XBD_ACK_SUCCESS == ack_res) {
          xbd_ipcomm_state = XBD_IDLE;
          break;
        } else {
          ipcomm_transmit_data(wait_seq_no);
        }
      }
      break;
    default:
      XBD_debugOut(IPCOMMDBG "invalid state: ");
      XBD_debugOutHexByte(xbd_ipcomm_state);
      XBD_debugOut("\n");
      break;
  } 
}
