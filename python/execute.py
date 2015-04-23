#! /usr/bin/env python3

import logging.config

import xbx.config as xbxc
import xbx.run as xbxr
import xbx.util as xbxu
import xbx.database as xbxdb

CONFIG_PATH="config.ini"

def main():
    logging.config.fileConfig("logging.ini", disable_existing_loggers=False)
    xbxdb.init(xbxu.get_db_path(CONFIG_PATH))
    s = xbxdb.scoped_session()

    config = xbxc.Config(CONFIG_PATH)

    logging.config.fileConfig("logging.ini", disable_existing_loggers=False)
    config = xbxc.Config("config.ini")

    rs = xbxr.RunSession(config)
    rs.init_xbh()
    rs.runall()
    s.add(rs)
    s.commit()




if __name__ == "__main__":
    main()
