#! /bin/python
import sys
sys.path.insert(0,'..')
import xbh as xbhpkg
import logging.config

logging.config.fileConfig("logging.ini", disable_existing_loggers=False)
xbh = xbhpkg.Xbh()
xbh.upload_prog("xbdprog.hex")
#xbh.switch_to_app()
