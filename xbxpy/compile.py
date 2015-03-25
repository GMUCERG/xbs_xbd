#! /usr/bin/env python3

import logging.config
import sys
import multiprocessing as mp
import os

import xbx.util as xbxu
import xbx.config as xbxc
import xbx.build as xbxb
import xbx.database as xbxdb


CONFIG_PATH="config.ini"


def main():
    logging.config.fileConfig("logging.ini", disable_existing_loggers=False)
    xbxdb.init(xbxu.get_db_path(CONFIG_PATH))
    s = xbxdb.scoped_session()

    config = xbxc.Config(CONFIG_PATH)

    bs = xbxb.BuildSession(config)
    bs.buildall()


    s.add(bs)
    s.commit()




if __name__ == "__main__":
    main()
