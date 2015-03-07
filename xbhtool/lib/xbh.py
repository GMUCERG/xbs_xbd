#! /usr/bin/python3
import socket
import re

XBH_CMD_LEN = 8
# Always 2 bytes
PROTO_VERSION = '05'

_CALC_TIMEOUT = 5*60

class XbhError(RuntimeError):
    def __init__(self, msg):
        super(XbhError, self).__init__(msg)


class Xbh:
    _sock = None
    _cmd_pending = None
    _bl_mode = None
    _timeout = None
    verbose = None

    
    def __init__(self, host="xbh", port=22595, src_host='', timeout=10, verbose = False):
        self._sock = socket.create_connection((host,port), timeout, (src_host, 0))
        self._timeout = timeout
        self.verbose = False

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
        msg_len = '{:04x}'.format(XBH_CMD_LEN+len(data))
        msg = (msg_len + 'XBH'+PROTO_VERSION+command+'r').encode()+data
        self._sock.sendall(msg);
        self._cmd_pending = command

    def _xbh_response(self):
        size = int(self._recvall(4),16)
        msg = self._recvall(size)
        match = re.match(
                r'^XBH([0-9]{2})'+self._cmd_pending+r'([aof])',
                msg.decode())

        if match == None:
            raise XbhError("Received invalid answer to " +
                    self._cmd_pending+": "+msg+".")

        version = match.group(1)
        status = match.group(2)

        if version.encode() != PROTO_VERSION:
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
        if self.bl_mode:
            self.switch_to_app()
        return
    
    def upload_pg(self, hexaddr, data):
        self.req_bl()

        if self.verbose:
            self.log("Uploading a page to address "+hexaddr)

        self._exec("cd",hexaddr.encode()+data)
        self._xbh_response()

    def exec_and_time(self):
        self.req_app()
        if self.verbose:
            self.log("Executing code")

        self._sock.set_timeout(_CALC_TIMEOUT)
        self._exec("ex")
        self._xbh_response()
        self._sock.set_timeout(_timeout)

    def upload_param(self, hextype, hexaddr, data):
        self.req_app()
        if self.verbose:
            self.log("Uploading parameters to address "+hexaddr)
        self._exec("dp",hextype.encode()+hexaddr.encode()++data)
        self._xbh_response()

    def get_timings(self):
        if self.verbose:
            self.log("Downloading timing results")

        self._exec("rp")
        msg = self._xbh_response().decode()
        match = re.match(r"^([0-9A-F]{8})([0-9A-F]{8})([0-9A-F]{8})$", msg)
        seconds      = int(match.group(1),16)
        frac         = int(match.group(2),16)
        frac_per_sec = int(match.group(3),16)
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
        else
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

        self._sock.set_timeout(_CALC_TIMEOUT)
        self._exec("ex")
        self._xbh_response()
        self._sock.set_timeout(_timeout)


    def get_results(self):
        self.req_app()
        if self.verbose:
            self.log("Downloading results")

        self._exec("ur")
        msg = self._xbh_response()
        return msg.decode()

    def get_stack_usage(self):
        self.req_app()
        if self.verbose:
            self.log("Getting Stack Usage")

        self._exec("su")
        msg = self._xbh_response()

        match = re.match(r"^.?([0-9A-F]{7,8})$", msg)
        
        return int(match.group(1),16)
        

    def get_timing_cal(self):
        self.req_bl()

        if self.verbose:
            self.log("Getting Timing Calibration")

        self._exec("tc")
        msg = self._xbh_response()

        match = re.match(r"^.?([0-9A-F]{7,8})$", msg)
        cycles = int(match.group(1),16)

        if self.verbose:
            self.log("Received "+cycles+" XBD clock cycles")

        return cycles







def main():
    xbh = Xbh()
    rev,mac = xbh.get_xbh_rev()
    print("XBH Git Rev : "+rev)
    print("XBH MAC Addr: "+mac)
    
if __name__ == "__main__":
    main()










    





        


