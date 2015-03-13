#! /usr/bin/env python3
import sys
sys.path.insert(0,'..')

import unittest.case

import xbx
import pprint

# Test with Keccakc512
class XbxTest(unittest.TestCase):
    verbose = True
    config = None
    def setUp(self):
        self.config = xbx.Config("test_config.ini")

    def tearDown(self):
        pass

    def test_settings(self):
        self.assertEqual(self.config.clock_hz, 16000000)
        self.assertEqual(self.config.pagesize, 1024)
        #pp = pprint.PrettyPrinter(indent=4)
        #pp.pprint(config.__dict__)
        ##pp.pprint(config.algorithms)
        #for k,v in config.algorithms.items():
        #    pp.pprint(v.__dict__)

    def test_impl(self):
        self.assertEqual(1, len(self.config.algorithms['0hash'].impls))

    def test_checksum(self):
        self.assertEqual("995ec6efa3a2e23e34c2b0fba46e55fd634c414173019d6437858741a70a82f2", 
                self.config.algorithms['keccakc512'].checksumsmall) 



if __name__ == '__main__':
    unittest.main()
