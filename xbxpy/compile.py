#! /usr/bin/env python3

import logging.config
import sys
import multiprocessing as mp
import os

import xbx.config as xbxc
import xbx.build as xbxb
import xbx.database as xbxdb



def main():
    logging.config.fileConfig("logging.ini", disable_existing_loggers=False)
    xbxdb.init("data.db")
    s = xbxdb.scoped_session()

    config = xbxc.Config("config.ini")


    bs = xbxb.BuildSession(config)
    bs.buildall()


    s.add(bs)
    s.commit()




if __name__ == "__main__":
    main()
