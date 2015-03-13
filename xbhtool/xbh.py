#! /usr/bin/python3
import binascii
import os
import re
import socket
import struct

import prog_reader

# Maximum segment size, assuming IPv6 and all TCP options (for worst-case)
# 1500 Ethernet payload - 40 IPv6 - 60 TCP (normally 20) = 1400
# 
# XBH can handle up to 1500
XBH_MAX_PAYLOAD = 1400

# Always 2 bytes
PROTO_VERSION = '05'

_XBH_CMD_LEN = 8
_CALC_TIMEOUT = 5*60


# Replace object w/ Enum when 3.4 is current
class TypeCode(object):# {{{
    HASH = 1
    AEAD = 2
# }}}

class XbhError(RuntimeError):# {{{
    def __init__(self, msg):
        super(XbhError, self).__init__(msg)
# }}}

class Xbh:# {{{
    _sock = None
    _cmd_pending = None
    _bl_mode = None
    _timeout = None
    verbose = None
    page_size = None
    xbd_hz = None

    
    def __init__(self, host="xbh", port=22595, page_size = 1024, xbd_hz=16000000, timeout=100000, verbose = True, src_host=''):
        self._sock = socket.create_connection((host,port), timeout, (src_host, 0))
        self._timeout = timeout
        self.verbose = verbose
        self.page_size = page_size
        self.xbd_hz = xbd_hz

    def __del__(self):
        if self._sock:
            self._sock.close()

    def log(self, msg):
        print(msg)

    @property
    def bl_mode(self):
        return self._bl_mode

    def _recvall(self, size):
        buf = bytearray()
        while(size > 0):
            msg = self._sock.recv(size)
            buf += msg
            size -= len(msg)
        return buf


    def _exec(self, command, data = b''):
        if len(data)+len(command) > XBH_MAX_PAYLOAD:
            raise ValueError("Command and payload too large")
        msg_len = '{:04x}'.format(_XBH_CMD_LEN+len(data))
        msg = (msg_len + 'XBH'+PROTO_VERSION+command+'r').encode()+data
        self._sock.sendall(msg);
        self._cmd_pending = command

    def _xbh_response(self):
        msg = self._recvall(4)
        size = int(msg,16)
        msg = self._recvall(size)
        match = re.match(
                r"^XBH([0-9]{2})"+self._cmd_pending+r"([aof])",
                msg[0:8].decode())

        if match == None:
            raise XbhError("Received invalid answer to " +
                    self._cmd_pending+": "+msg.decode()+".")

        version = match.group(1)
        status = match.group(2)

        if version != PROTO_VERSION:
            raise XbhError("XBH protocol version was " + version + 
                    ", this tool requires "+PROTO_VERSION+".")

        if status == 'a':
            if self.verbose:
                self.log("Received 'a'ck")
        elif status == 'o':
            if self.verbose:
                self.log("Received 'o'kay")
        elif status == 'f':
            raise XbhError("Received 'f'ail")

        # Return bytes after 8-byte command header
        return msg[8:]


    def switch_to_app(self):
        if self.verbose:
            self.log("Switching to application mode");
        self._exec("sa")
        self._xbh_response()
        self._bl_mode = False


    def switch_to_bl(self):
        if self.verbose:
            self.log("Switching to bootloader mode")

        self._exec("sb")
        msg = self._xbh_response()
        self._bl_mode = True

    def req_bl(self):
        if self.bl_mode:
            return
        
        self.switch_to_bl()

    def req_app(self):
        if self.bl_mode != False:
            self.switch_to_app()
        return
    
    def exec_and_time(self):
        self.req_app()
        if self.verbose:
            self.log("Executing code")

        self._sock.settimeout(_CALC_TIMEOUT)
        self._exec("ex")
        self._xbh_response()
        self._sock.settimeout(self._timeout)

    def get_timings(self):
        if self.verbose:
            self.log("Downloading timing results")

        self._exec("rp")
        msg = self._xbh_response()
        seconds, frac, frac_per_sec = struct.unpack("!III", msg)
        if self.verbose:
            self.log("Receive {} {} {}".format(
                seconds, frac, frac_per_sec))

        return seconds, frac, frac_per_sec

    def reset_xbd(self, reset_active):
        param = 'y' if reset_active else 'n'
        if self.verbose:
            self.log("Setting XBD reset to "+reset_active)

        self._exec("rc", param.encode())
        self._xbh_response()

        self._bl_mode = None

    def get_status(self):
        if self.verbose:
            self.log("Getting status of XBH")

        self._exec("st")

        msg = self._xbh_response()

        if msg.decode() == "XBD"+PROTO_VERSION+"BLo":
            self._bl_mode = True
        elif msg.decode() == "XBD"+PROTO_VERSION+"AFo":
            self._bl_mode = False
        else:
            self._bl_mode = None

        return self._bl_mode


    def get_xbh_rev(self):
        if self.verbose:
            self.log("Getting git revision (and MAC address) of XBH");

        self._exec("sr")
        msg = self._xbh_response().decode().split(',')
        rev = msg[0]
        mac = msg[1]

        return rev, mac

    def get_bl_rev(self):
        self.req_bl()

        if self.verbose:
            self.log("Getting git revision of XBD bootloader");

        self._exec("tr")
        msg = self._xbh_response().decode()

        return msg


    def set_comm_mode(self, mode):
        # Currently don't support anything but I2C
        raise NotImplementedError("Setting xbh<->xbd communication mode not implemented. Always I2C.")

        if None == mode.match("[IUOTE]"):
            raise NotImplementedError("Mode "+mode+" not supported")

        if self.verbose:
            self.log("Setting communication mode to "+mode)

        self._exec("sc",mode)
        self._xbh_response()


    def calc_checksum(self):
        self.req_app()
        self.log("Calculating checksum")

        self._sock.settimeout(_CALC_TIMEOUT)
        self._exec("cc")
        self._xbh_response()
        self._sock.settimeout(self._timeout)


    def get_results(self):
        self.req_app()
        if self.verbose:
            self.log("Downloading results")

        self._exec("ur")
        msg = self._xbh_response()
        return binascii.hexlify(msg)

    def get_stack_usage(self):
        self.req_app()
        if self.verbose:
            self.log("Getting Stack Usage")

        self._exec("su")
        msg = self._xbh_response()

        stack = struct.unpack("!I", msg)[0]

        if self.verbose:
            self.log("Used "+stack+" bytes on stack")
        
        return stack
        

    def get_timing_cal(self):
        self.req_bl()

        if self.verbose:
            self.log("Getting Timing Calibration")

        self._exec("tc")
        msg = self._xbh_response()

        cycles = struct.unpack("!I", msg)[0]

        if self.verbose:
            self.log("Received "+str(cycles)+" XBD clock cycles")

        return cycles


    def _upload_pages(self, addr, data):
        #if data % page_size != 0:
        #    raise ValueError("data length must be integer multiple of pagesize")
            
        self.req_bl()

        #if self.verbose:
        #    self.log("Uploading a page to address "+hexaddr)

        self._exec("cd",struct.pack("!I",addr)+data)
        self._xbh_response()

    def _upload_data(self, typecode, addr, data):
        self.req_app()
        #if self.verbose:
        #    self.log("Uploading parameters to address "+hexaddr)
        self._exec("dp",struct.pack("!II",typecode,addr)+data)
        self._xbh_response()


    def upload_prog(self,filename, program_type=prog_reader.ProgramType.IHEX):
        reader = prog_reader.ProgramReader(filename, program_type, self.page_size)
        for addr, data in reader.areas.items():
            # Max payload - command length - 4 bytes for size
            MAX_DATA = XBH_MAX_PAYLOAD - _XBH_CMD_LEN - 4
            offset = 0
            length = len(data)
            block_size = (MAX_DATA//self.page_size)
            upload_bytes = block_size*self.page_size

            if length == 0:
                continue

            if self.verbose:
                self.log("Uploading {} bytes {} page(s) at a time starting at {}"
                        .format(length, block_size, hex(addr)))
                pass # fix autoindent wonkyness

            while offset < length:
                self._upload_pages(addr+offset, data[offset:offset+upload_bytes])
                if self.verbose:
                    uploaded_bytes = (upload_bytes if upload_bytes <
                            (length - offset) else (length - offset))
                    self.log("Uploading {} bytes starting at {}"
                            .format(uploaded_bytes,hex(addr+offset)))
                offset += upload_bytes


    def upload_param(self, data=1536, typecode=TypeCode.HASH):
            # Max payload - command length - 4 bytes for size - 4 bytes for type
            MAX_DATA = XBH_MAX_PAYLOAD - _XBH_CMD_LEN - 4 -4

            if type(data) == int:
                data = os.urandom(data)

            length = len(data)
            offset = 0
            
            if self.verbose:
                self.log("Uploading {} bytes {} at a time"
                        .format(length, MAX_DATA))
                pass # fix autoindent wonkyness

            while offset < length:
                self._upload_data(offset, data[offset:offset+MAX_DATA])
                if self.verbose:
                    uploaded_bytes = (MAX_DATA if MAX_DATA <
                            (length - offset) else (length - offset))
                    self.log("Uploading {} bytes starting at {}"
                            .format(uploaded_bytes,hex(addr+offset)))
                offset += MAX_DATA


# }}}




                        
                

                

                







        



#if __name__ == "__main__":
#    xbh = Xbh()
#    xbh.calc_checksum()
#
#    print(xbh.get_results())





    





        


