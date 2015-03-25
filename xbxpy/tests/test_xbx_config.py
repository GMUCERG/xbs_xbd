#! /usr/bin/env python3
import sys
sys.path.insert(0,'..')

import unittest.case
import pprint

import xbx.config
import xbx.util

# Test with Keccakc512
class XbxTest(unittest.TestCase):
    verbose = True
    config = None
    def setUp(self):
        self.config = xbx.config.Config("test_config.ini")

    def tearDown(self):
        pass

    def test_settings(self):
        self.assertEqual(self.config.platform.clock_hz, 16000000)
        self.assertEqual(self.config.platform.pagesize, 1024)
        #pp = pprint.PrettyPrinter(indent=4)
        #pp.pprint(config.__dict__)
        ##pp.pprint(config.algorithms)
        #for k,v in config.algorithms.items():
        #    pp.pprint(v.__dict__)

    def test_impl(self):
        self.assertEqual(1, len(self.config.primitives[0].impls))

    def test_checksumsmall(self):
        self.assertEqual("995ec6efa3a2e23e34c2b0fba46e55fd634c414173019d6437858741a70a82f2", 
                self.config.primitives[1].checksumsmall) 

    def test_stable_checksum(self):
        c_hash = xbx.util.hash_obj(self.config)
        for i in range(10):
            config = xbx.config.Config("test_config.ini")
            self.assertEqual(c_hash, xbx.util.hash_obj(config))
            
    # TODO: Add test to check if changes in platform still result in only one
    # platform run and platform persisted in db

    # TODO: Add test to check if changes in impl still result in only one
    # impl run and platform persisted in db

    # TODO: Add test to check if number of implementations is correct

    # TODO: Add test to check if proper number of builds run

    # TODO: Check if build output sizes is nonzero, i.e. HAL properly linked in



if __name__ == '__main__':
    unittest.main()
