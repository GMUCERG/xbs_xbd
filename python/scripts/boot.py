#! /bin/python
import sys
import logging.config
sys.path.insert(0,'..')
import xbh as xbhpkg

logging.config.fileConfig("logging.ini", disable_existing_loggers=False)
xbh = xbhpkg.Xbh()
xbh.switch_to_bl()
