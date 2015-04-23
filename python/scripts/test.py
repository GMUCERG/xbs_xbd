#! /bin/python
import xbh as xbhpkg

xbh = xbhpkg.Xbh()
#xbh.switch_to_app()
xbh.calc_checksum()
print(xbh.get_results())
