#! /usr/bin/env python3
import sys
sys.path.insert(0,'..')

import unittest.case

import xbh
import xbx.config as xbxc


# Test with Keccakc512
class XbhTest(unittest.TestCase):
    config = None
    xbh = None
    verbose = False
    def setUp(self):
        self.config = xbxc.Config("test_config.ini")
        c = self.config
        self.xbh = xbh.Xbh(
                    c.xbh_addr, c.xbh_port, 
                    c.platform.pagesize,
                    c.platform.clock_hz)
    def tearDown(self):
        del self.config
        del self.xbh

    def test_switch(self):
        self.assertEqual(self.xbh.bl_mode, None)
        self.xbh.req_bl()
        self.assertEqual(self.xbh.bl_mode, True)
        self.xbh.req_app()
        self.assertEqual(self.xbh.bl_mode, False)
        self.xbh.req_bl()
        self.assertEqual(self.xbh.bl_mode, True)

    def test_calibration(self):
        cycles = self.xbh.get_timing_cal()
        self.assertAlmostEqual(cycles, self.config.platform.clock_hz, delta=1000)
        





if __name__ == '__main__':
    unittest.main()
