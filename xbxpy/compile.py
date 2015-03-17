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
    threads = []
    processes = []

    queue = mp.JoinableQueue()
    exit_event = mp.Event()

    for p in CONFIG.primitives:
        for i in p.impls:
            index = 0
            for compiler in CONFIG.platform.compilers:
                build = xbxb.Build(CONFIG, compiler['cc'],  compiler['cxx'], 
                        index, i )
                builds += build,
                index += 1
                if CONFIG.one_compiler:
                    break


#    def worker(queue, exit_event):
#        while not exit_event.is_set():
#            build = queue.get()
#            build.compile()
#            queue.task_done()
#
#    for i in range(0, CPU_COUNT):
#        p = mp.Process(target=worker, args=(queue, exit_event))
#        p.start()
#        processes += p,
#
#
#    for b in builds:
#        queue.put(b)
#
#    queue.join()
#    exit_event.set()
#    for p in processes:
#        p.join()
#



    #def th(b):
    #    b.compile()
    #for b in builds:
    #    threading.Thread(target=th, args=(b,)).start()
    for b in builds:
        b.compile()



if __name__ == "__main__":
    setup_logging()
    build()
