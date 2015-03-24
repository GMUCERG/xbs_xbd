#! /usr/bin/env python3

import logging.config

import xbx.config as xbxc
import xbx.build as xbxb
import xbx.run as xbxr
import xbx.data as xbxd

def main():
    logging.config.fileConfig("logging.ini", disable_existing_loggers=False)
    config = xbxc.Config("config.ini")

    db = xbxd.Database(config)

    rs = xbxr.RunSession(config, db)

    rs.init_xbh()
    rs.drift_measurements()




if __name__ == "__main__":
    main()
