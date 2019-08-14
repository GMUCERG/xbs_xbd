#! /usr/bin/env python3
import binascii
import sys
import os
import re
import socket
import struct
import logging
import time
from decimal import Decimal

import prog_reader
import xbh


# Maximum segment size, assuming IPv6 and all TCP options (for worst-case)
# 1500 Ethernet payload - 40 IPv6 - 60 TCP (normally 20) = 1400
#
# XBH can handle up to 1500
XBH_MAX_PAYLOAD = 1400

# Always 2 bytes
PROTO_VERSION = '05'

FAIL_CHECKSUM = -62
FAIL_DEFAULT = -63
FAIL_TYPE_MISMATCH = -61


_XBH_CMD_LEN = 8

_logger=logging.getLogger(__name__)


# Replace object w/ Enum when 3.4 is current
class TypeCode(object):
    HASH = 1
    AEAD = 2
    KEM = 3
    SIGN = 4

class Error(Exception):
    pass

class XbdFailError(xbh.Error):
    pass

class ChecksumFailError(xbh.Error):
    pass

class XbhValueError(xbh.Error, ValueError):
    pass

class RetryError(Error):
    pass

class TimeoutError(xbh.Error):
    pass

class HardwareError(xbh.Error):
    pass


class attempt:
    """Decorator to attempt XBH operation for tries tries

    Parameters:
        xbh         Either a string containing the name of an attribute to
                    Xbh, or an instance of Xbh. None if no reconnection is to be
                    attempted

        tries       Number of attempts

        raise_err   If True will allow exception to bubble up

    """
    def __init__(self, xbh_var=None, tries=3, raise_err=False):
        self.xbh_var = xbh_var
        self.tries = tries
        self.raise_err = raise_err

    def __call__(self, func):
        xbh_var = self.xbh_var
        tries = self.tries
        raise_err = self.raise_err

        caught_errors = (XbdFailError, HardwareError)

        if isinstance(xbh_var,str):

            def func_wrapper(self, *args, **kwargs):
                xbh = getattr(self, xbh_var)
                ex = None
                for t in range(tries):
                    try:
                        return func(self, *args, **kwargs)
                    except caught_errors as e:
                        time.sleep(0.5)
                        try:
                            xbh.reconnect()
                            xbh.reset_xbd()
                        except:
                            pass
                        ex = e
                        _logger.error(str(e))
                if raise_err:
                    raise RetryError(
                            "Errors exceeded retry count ["+str(tries)+"]") from ex
            return func_wrapper
        else:
            def func_wrapper(*args, **kwargs):
                xbh = xbh_var
                ex = None
                for t in range(tries):
                    try:
                        return func(*args, **kwargs)
                    except caught_errors as e:
                        time.sleep(0.5)
                        try:
                            xbh.reconnect()
                            xbh.reset_xbd()
                        except:
                            pass
                        ex = e
                        _logger.error(str(e))
                if raise_err:
                    raise RetryError(
                            "Errors exceeded retry count ["+tries+"]") from ex
            return func_wrapper


class Xbh:


    def __init__(self, host="xbh", port=22595, page_size = 1024,
            xbd_hz=16000000, timeout=999999, sock_timeout=30, src_host=''):
        self._connargs = ((host,port), sock_timeout, (src_host, 0))
        self._sock = socket.create_connection(*self._connargs)
        self.timeout = timeout
        self._cmd_pending = None
        self._bl_mode = None
        self.page_size = page_size
        self.xbd_hz = xbd_hz
        self.set_power_gain(25)
        self.args_start_addr = 0

    def __del__(self):
        try:
            self._sock.close()
        except:
            pass

    def reconnect(self):
        try:
            self._sock.shutdown()
            self._sock.close()
        except:
            pass

        self._sock = socket.create_connection(*self._connargs)

    @property
    def bl_mode(self):
        return self._bl_mode


# Internal functions
    def _recvall(self, size):
        try:
            buf = bytearray()
            while(size > 0):
                msg = self._sock.recv(size)
                buf += msg
                size -= len(msg)
            return buf
        except socket.timeout as e:
            raise Error("Socket timeout") from e


    def _exec(self, command, data = b''):
        if len(data)+len(command) > XBH_MAX_PAYLOAD:
            raise ValueError("Command and payload too large")
        msg_len = '{:04x}'.format(_XBH_CMD_LEN+len(data))
        msg = (msg_len + 'XBH'+PROTO_VERSION+command+'r').encode()+data
        self._sock.sendall(msg);
        self._cmd_pending = command

    def _xbh_response(self):
        start_time = time.time()
        while True:
            if time.time()-start_time > self.timeout:
                raise TimeoutError("Operation timed out")

            msg = self._recvall(4)
            size = int(msg,16)
            msg = self._recvall(size)


            # Keep going if keepalive received
            if msg[0:8].decode() == ("XBH"+PROTO_VERSION+"kao"):
                _logger.debug("Received 'k'eep'a'live")
                continue

            match = re.match(
                    r"^XBH([0-9]{2})"+self._cmd_pending+r"([aof])",
                    msg[0:8].decode())

            if match == None:
                raise XbhValueError("Received invalid answer to " +
                        self._cmd_pending+": "+msg.decode()+".")

            version = match.group(1)
            status = match.group(2)

            if version != PROTO_VERSION:
                raise XbhValueError("XBH protocol version was " + version +
                        ", this tool requires "+PROTO_VERSION+".")

            if status == 'a':
                    _logger.debug("Received 'a'ck")
            elif status == 'o':
                    _logger.debug("Received 'o'kay")
            elif status == 'f':
                raise XbdFailError("Received 'f'ail")

            # Return bytes after 8-byte command header
            return msg[8:]

    def _upload_code_pages(self, addr, data):
        if len(data) % self.page_size != 0 and len(data) > self.page_size:
            raise ValueError("data length must be integer multiple of pagesize")

        self.req_bl()

        #if self.verbose:
        #    _logger.info("Uploading a page to address "+hexaddr)

        self._exec("cd",struct.pack("!I",addr)+data)
        self._xbh_response()

    def _upload_data(self, addr, typecode, data):
        self.req_app()
        _logger.debug("Uploading parameters to address "+hex(addr))
        self._exec("dp",struct.pack("!II",addr, typecode)+data)
        self._xbh_response()



# Low-level interfaces
    def switch_to_app(self):
        _logger.debug("Switching to application mode");
        self._exec("sa")
        self._xbh_response()
        self._bl_mode = False


    def switch_to_bl(self):
        _logger.debug("Switching to bootloader mode")

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

    def set_power_gain(self, gain):
        _logger.debug("Setting XBP gain to %u" % gain)
        self._exec("pg",struct.pack("!I",gain))
        self._xbh_response()


    def reset_xbd(self, reset_active):
        param = 'y' if reset_active else 'n'
        _logger.debug("Setting XBD reset to "+reset_active)

        self._exec("rc", param.encode())
        self._xbh_response()

        self._bl_mode = None

    def get_status(self):
        _logger.debug("Getting status of XBH")

        self._exec("st")

        msg = self._xbh_response()

        if msg.decode() == "XBD"+PROTO_VERSION+"BLo":
            self._bl_mode = True
        elif msg.decode() == "XBD"+PROTO_VERSION+"AFo":
            self._bl_mode = False
        else:
            self._bl_mode = None

        return self._bl_mode

    def set_comm_mode(self, mode):
        # Currently don't support anything but I2C
        raise NotImplementedError("Setting xbh<->xbd communication mode not implemented. Always I2C.")

        if None == mode.match("[IUOTE]"):
            raise NotImplementedError("Mode "+mode+" not supported")

        _logger.debug("Setting communication mode to "+mode)

        self._exec("sc",mode)
        self._xbh_response()

    def _exec_and_time(self):
        self.req_app()
        _logger.debug("Executing code")

        self._exec("ex")
        self._xbh_response()

    def _calc_checksum(self):
        self.req_app()
        _logger.debug("Calculating checksum")

        self._exec("cc")
        self._xbh_response()

    def _get_timings(self):
        """Returns seconds, frac, frac_per_sec"""
        _logger.debug("Downloading timing results")

        self._exec("rp")
        msg = self._xbh_response()
        seconds, frac, frac_per_sec = struct.unpack("!III", msg)
        _logger.debug("Receive {} {} {}".format(
                seconds, frac, frac_per_sec))

        return seconds, frac, frac_per_sec
    
    def get_power(self):
        """Returns Power"""
        _logger.debug("Downloading power measurements")

        self._exec("pw")
        msg = self._xbh_response()
        #measured_time = self.get_measured_time(timings)
        #gain = self.set_power_gain()
        # power, current, voltage, avgpwr, maxpwr = struct.unpack("!IIIII", msg)
        # _logger.debug("Receive {} {} {} {} {}".format(
        #         power, current, voltage, avgpwr, maxpwr))

        # has to use values from config.ini!!!

        avgpwr, maxpwr, cnt_overflow = struct.unpack("!III", msg)
        _logger.debug("Receive {} {} {}".format(
                avgpwr, maxpwr, cnt_overflow))
        # might have to move computation to run.py if I can't get config.ini values here
        
        ##volatge is currently stored in avgpwr
        #avgpwr=float((avgpwr*3.3)/(4096*200))
        ##power = V*I
        #avgpwr=float(avgpwr*3.3)
        #maxpwr=float((maxpwr*3.3)/(4096*200))
        #maxpwr=float( maxpwr*3.3)
        
        ##for now assuming cnt_overflow as total_energy
        ## cnt_overflow=float(avg_power/measured_time)
        
        return avgpwr, maxpwr, cnt_overflow

    def _get_stack_usage(self):
        self.req_app()
        _logger.debug("Getting Stack Usage")

        self._exec("su")
        msg = self._xbh_response()

        stack, = struct.unpack("!I", msg)

        _logger.debug("Used "+str(stack)+" bytes on stack")

        return stack


    def _get_results(self):
        """Gets operation output from exec_and_time()

        Resets XBD for new run, so run after get_stack_usage() and get_timings()
        """
        self.req_app()
        _logger.debug("Downloading results")

        self._exec("ur")
        fmt = "!i"
        msg = self._xbh_response()
        retval, = struct.unpack_from(fmt, msg)
        data = msg[struct.calcsize(fmt):]
        return retval, data



    def get_timing_cal(self):
        """Returns cycles counted by XBD"""
        self.req_bl()

        _logger.debug("Getting Timing Calibration")

        self._exec("tc")
        msg = self._xbh_response()

        cycles = struct.unpack("!I", msg)[0]

        _logger.debug("Received "+str(cycles)+" XBD clock cycles")

        return cycles

    def get_xbh_rev(self):
        _logger.debug("Getting git revision (and MAC address) of XBH");

        self._exec("sr")
        msg = self._xbh_response()
        msg = msg.decode().split(',')
        rev = msg[0]
        mac = msg[1].strip('\0')

        return rev, mac

    def get_bl_rev(self):
        self.req_bl()

        _logger.debug("Getting git revision of XBD bootloader");

        self._exec("tr")
        msg = self._xbh_response().decode()

        return msg


# High level interfaces
    @staticmethod
    def get_measured_time(timings):
        """Gets measured time in nanoseconds"""
        seconds, fractions, frac_per_sec = timings
        measured_time = int(round(10**9*(seconds + fractions/frac_per_sec)+0.5))
        return measured_time

    def get_measured_cycles(self, timings):
        # add 0.5 then cast to int to get rounded integer
        seconds, fractions, frac_per_sec = timings
        measured_cycles = int((seconds + fractions/frac_per_sec)*self.xbd_hz+0.5)
        return measured_cycles

    def execute(self):
        self._exec_and_time()
        timings = self._get_timings()
        stack = self._get_stack_usage()
        results = self._get_results()
        return results, timings, stack

    def calc_checksum(self):
        try:
            self._calc_checksum()
            timings = self._get_timings()
            stack = self._get_stack_usage()
            results = self._get_results()
            return results, timings, stack
        except XbdFailError as e:
            retval, msg = self._get_results();
            if retval == FAIL_CHECKSUM:
                raise ChecksumFailError("Test failure: "+
                                        msg.decode().strip('\0')) from e
            else:
                raise


    def upload_prog(self, filename, program_type=prog_reader.ProgramType.IHEX):
        """Uploads provided file to XBH to upload to XBD"""
        reader = prog_reader.ProgramReader(filename, program_type, self.page_size)
        for addr, data in reader.areas.items():

            # Commented out block below is for when XBD supports more than one
            # page at a time
            ## Max payload - command length - 4 bytes for size
            #MAX_DATA = XBH_MAX_PAYLOAD - _XBH_CMD_LEN - 4
            #offset = 0
            #length = len(data)
            #block_size = (MAX_DATA//self.page_size)
            #upload_bytes = block_size*self.page_size

            offset = 0
            length = len(data)
            block_size = 1
            upload_bytes = block_size*self.page_size

            if length == 0:
                continue

            _logger.debug("Uploading {} bytes {} page(s) at a time starting at {}"
                          .format(length, block_size, hex(addr)))

            while offset < length:
                self._upload_code_pages(addr+offset, data[offset:offset+upload_bytes])
                uploaded_bytes = (upload_bytes if upload_bytes <
                        (length - offset) else (length - offset))
                _logger.debug("Uploading {} bytes starting at {}"
                        .format(uploaded_bytes,hex(addr+offset)))
                offset += upload_bytes

            # Grab the block right after the program file
            self.args_start_addr = addr + offset


    def upload_static_param(self, data):
        """Uploads large static arguments to flash"""

        addr = self.args_start_addr
        assert addr != 0, "Invalid argument flash address"

        offset = 0
        length = len(data)
        block_size = 1
        upload_bytes = block_size*self.page_size

        if length == 0:
            return

        _logger.debug("Uploading {} bytes {} page(s) at a time starting at {}"
                    .format(length, block_size, hex(addr)))

        while offset < length:
            self._upload_code_pages(addr+offset, data[offset:offset+upload_bytes])
            uploaded_bytes = (upload_bytes if upload_bytes <
                    (length - offset) else (length - offset))
            _logger.debug("Uploading {} bytes starting at {}"
                    .format(uploaded_bytes,hex(addr+offset)))
            offset += upload_bytes

        return self.args_start_addr


    def upload_param(self, data, typecode=TypeCode.HASH):
        """Uploads buffer to xbh

        """
        # Max payload - command length - 4 bytes for size - 4 bytes for type
        MAX_DATA = XBH_MAX_PAYLOAD - _XBH_CMD_LEN - 4 -4


        length = len(data)
        offset = 0

        _logger.debug("Uploading {} bytes {} at a time"
                    .format(length, MAX_DATA))

        while True:
            uploaded_bytes = (MAX_DATA if MAX_DATA <
                    (length - offset) else (length - offset))
            _logger.debug("Uploading {} bytes starting at {}"
                    .format(uploaded_bytes,hex(offset)))
            self._upload_data(offset, typecode,
                              data[offset:offset+MAX_DATA])
            offset += MAX_DATA
            if not offset < length:
                break

    def measure_timing_error(self):
        cycles = self.get_timing_cal()
        timings = self._get_timings()
        if cycles == 0:
            raise xbh.ValueError("Cycle count 0!")
        measured_cycles = self.get_measured_cycles(timings)
        if measured_cycles == 0:
            raise xbh.ValueError("Measured cycle count 0!")
        abs_error = measured_cycles - cycles
        # TODO: Divide by measured cycles or by nominal cycles?
        rel_error = abs_error/cycles

        return abs_error, rel_error, cycles, measured_cycles




