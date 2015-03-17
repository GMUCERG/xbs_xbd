#! /usr/bin/env python3

import logging
import sys
import xbx

# Setup basic logging w/ DEBUG verbosity
logging.basicConfig(stream=sys.stdout, level=logging.DEBUG, format="%(levelname)s: %(buildid)s")

# For compile stuff, since it runs in parallel, distinguish between builds w/
# buildid
handler = logging.StreamHandler(sys.stdout)
handler.setFormatter(logging.Formatter("%(levelname)s: %(buildid)s %(message)s"))
comp_logger = logging.getLogger("xbx.Build")
comp_logger.addHandler(handler)
comp_logger.propagate = False

#FORMAT = "%(buildid)s %(message)s"



builds = []
config = xbx.Config("config.ini")

for p in config.primitives:
    for i in p.impls:
        index = 0
        for compiler in config.platform.compilers:
            build = xbx.Build(config, compiler['cc'],  compiler['cxx'], 
                    index, i , True)
            #build.compile()
            builds += build,
            index += 1

