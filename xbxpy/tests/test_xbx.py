#! /usr/bin/env python3
import sys
sys.path.insert(0,'..')

import os

import unittest.case
import pprint

import xbx.util as xbxu
import xbx.config as xbxc
import xbx.build as xbxb
import xbx.database as xbxdb


CONFIG_PATH="test_config.ini"

def setUpModule():
    db_path = ""
    xbxdb.init(db_path)


# Test with Keccakc512/Simple
class XbxConfigTest(unittest.TestCase):# {{{
    verbose = True
    config = None
    @classmethod
    def setUpClass(cls):
        pass

    def setUp(self):
        self.config = xbxc.Config(CONFIG_PATH)
        s = xbxdb.scoped_session()
        s.add(self.config)

    def tearDown(self):
        pass

    def test_settings(self):
        self.assertEqual(self.config.platform.clock_hz, 16000000)
        self.assertEqual(self.config.platform.pagesize, 1024)
        self.assertEqual(self.config.operation.name, "crypto_hash")

    def test_checksumsmall(self):
        s = xbxdb.scoped_session()
        primitive = s.query(xbxc.Primitive).filter(xbxc.Primitive.name == "keccakc512").one()
        self.assertEqual("995ec6efa3a2e23e34c2b0fba46e55fd634c414173019d6437858741a70a82f2",
                primitive.checksumsmall)

    def test_stable_checksum(self):
        self.assertEqual(xbxc.hash_config(CONFIG_PATH), self.config.hash)

    def test_validate_platform(self):
        platform = self.config.platform
        testname = os.path.join(platform.path, "test")

        self.assertTrue(platform.valid_hash)
        with open(testname, "w") as f:
            f.write("test")
        self.assertFalse(platform.valid_hash)
        os.remove(testname)
        self.assertTrue(platform.valid_hash)

    def test_validate_implementation(self):
        s = xbxdb.scoped_session()
        # Assume only one instance, since we nuked db earlier
        impls = s.query(xbxc.Implementation).filter(
            xbxc.Implementation.name == "simple",
            xbxc.Implementation.primitive_name == "keccakc512"
        ).one()

        impl = impls

        testname = os.path.join(impl.path, "test")

        self.assertTrue(impl.valid_hash)
        with open(testname, "w") as f:
            f.write("test")
        self.assertFalse(impl.valid_hash)
        os.remove(testname)
        self.assertTrue(impl.valid_hash)


# TODO: Test template platforms
# TODO: Add test to verify if config loads from db
# TODO: Add test to verify if config reloads if config.ini changes
# TODO: Add test to verify if new platform or impl is added if current checksum
# is obsolete
# TODO: Add test to check if proper number of builds run
# }}}

class XbxBuildTest(unittest.TestCase):
# TODO: Check if build output sizes is nonzero, i.e. HAL properly linked in
    pass





if __name__ == '__main__':
    unittest.main()
