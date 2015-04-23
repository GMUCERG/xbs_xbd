import collections
from lib import ihex
#from enum import Enum

# Subclass Enum when 3.4 is more widespread instead
class ProgramType(object):
    IHEX=0
    BIN=1


class ProgramReader:
    """Wrapper around ihex and binfile readers

    Seperates areas seperated by holes surrounding a whole page to be programmed seperately and zero
    fills holes that are within a page. Assumes pages are always aligned
    
    Attributes:
        areas   Dictionary of seperate contiguous pages to program, keyed and sorted by address"""

    areas = collections.OrderedDict()

    def __init__(self, filename, src_reader=ProgramType.IHEX, page_size=None):
        if src_reader == ProgramType.IHEX:
            addr_mask = 0xFFFFFFFF
            if page_size != None:
                addr_mask = 0xFFFFFFFF-(page_size-1)

            hexfile = ihex.IHex.read_file(filename)
            area_addrs = list(hexfile.areas.keys())

            last_area_addr = area_addrs[0]
            last_addr = last_area_addr

            self.areas[last_area_addr] = bytearray()

            for addr, data in hexfile.areas.items():
                hole_size = (addr - last_addr) - 1
                # Test for hole
                if hole_size > 0:
                    # if page size specified AND empty page exists in between
                    # we make a new area
                    if (page_size != None and
                            (addr_mask & addr) - (addr_mask & last_addr) > 1):
                        last_area_addr = addr
                        self.areas[last_area_addr] = bytearray()
                    #Otherwise, zero fill hole
                    else:
                        self.areas[last_area_addr] += b'\0'*hole_size

                self.areas[last_area_addr] += data
                last_addr = addr+len(data)

        else:
            raise ValueError("Program sources other than IHEX currently not supported")

        

