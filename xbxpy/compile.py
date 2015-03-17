#! /usr/bin/env python3

import logging
import sys
import xbx.config as xbxc
import xbx.build as xbxb
import multiprocessing as mp
import os

CPU_COUNT = mp.cpu_count()
CONFIG = xbxc.Config("config.ini")

def setup_logging():
    # Setup basic logging w/ DEBUG verbosity
    logging.basicConfig(stream=sys.stdout, level=CONFIG.loglevel, format="%(levelname)s: %(buildid)s")

    # For compile stuff, since it runs in parallel, distinguish between builds w/
    # buildid
    handler = logging.StreamHandler(sys.stdout)
    handler.setFormatter(logging.Formatter("%(levelname)s: %(buildid)s %(message)s"))
    comp_logger = logging.getLogger("xbx.build.Build")
    comp_logger.addHandler(handler)
    comp_logger.propagate = False

    #FORMAT = "%(buildid)s %(message)s"



def build():
    builds = []

    num_compilers = len(CONFIG.platform.compilers)
    for i in range(num_compilers):
        xbxb.build_hal(CONFIG, i)

    for p in CONFIG.primitives:
        for i in range(num_compilers):
            for j in p.impls:
                build = xbxb.Build(CONFIG, i, j )
                builds += build,

            if CONFIG.one_compiler:
                break

    #for b in builds:
    #    b.compile()

    q = mp.JoinableQueue()

    def worker(q):
        for build in iter(q.get, None):
            build.compile()
            q.task_done()

    processes = [mp.Process(target=worker, args=(q,)) for i in range(CPU_COUNT)]
    for p in processes:
        p.start()

    for b in builds:
        q.put(b)

    q.join()

    for p in processes:
        q.put(None)

    q.close()





    #def th(b):
    #    b.compile()
    #for b in builds:
    #    threading.Thread(target=th, args=(b,)).start()



if __name__ == "__main__":
    setup_logging()
    build()
