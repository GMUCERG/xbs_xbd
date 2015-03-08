#! /usr/bin/env python3
import xbh
import ihex
import numpy

from prog_reader import ProgramType, ProgramReader

# Maximum segment size, assuming IPv6 and all TCP options (for worst-case)
# 1500 Ethernet payload - 40 IPv6 - 60 TCP (normally 20) = 1400
# 
# XBH can handle up to 1500
XBH_MAX_PAYLOAD = 1400

class XbhTool:
    page_size = None
    xbh = None

    def __init__(self, xbh, page_size = 256):
        self.page_size = page_size
        self.xbh = xbh

    def upload(self,filename, program_type=ProgramType.IHEX):
        reader = ProgramReader(filename, program_type, page_size)
        for addr, data in reader.areas.items():







        



if __name__ == "__main__":
    xbhtool = XbhTool(xbh.Xbh(), 512)

    xbhtool.upload("xbhprog.hex")













