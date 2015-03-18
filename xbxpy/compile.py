#! /usr/bin/env python3

import logging.config
import sys
import multiprocessing as mp
import os

import xbx.config as xbxc
import xbx.build as xbxb

CPU_COUNT = mp.cpu_count()

def build(config):
    """Builds all targets specified in xbx config"""
    builds = []

    num_compilers = len(config.platform.compilers)

    for i in range(num_compilers):
        logging.info("compiler[{}] = {}".format(i,
            str(config.platform.compilers[i])))
        if config.one_compiler:
            break

    for i in range(num_compilers):
        xbxb.build_hal(config, i)
        if config.one_compiler:
            break

    for p in config.primitives:
        for j in p.impls:
            for i in range(num_compilers):
                build = xbxb.Build(config, i, j)
                builds += build,

                if config.one_compiler:
                    break
    
    if config.parallel_build:
        q_out = mp.Queue()
        q_in = mp.Queue()

        def worker(q_in, q_out):
            for build in iter(q_in.get, None):
                build.compile()
                q_out.put(build)

        processes = [mp.Process(target=worker, args=(q_out,q_in)) for i in range(CPU_COUNT+1)]
        for p in processes:
            p.start()

        for b in builds:
            q_out.put(b)

        # Clear out old build list, reobtain from queue with updated data
        num_builds = len(builds)
        builds = []
        for _ in range(num_builds):
            builds += q_in.get(),

        # Terminate processes
        for p in processes:
            q_out.put(None)
    else:
        for b in builds:
            b.compile()
    return builds

def save_build(sqlite, build_stats):
    """Saves build stats into sqlite connection given"""
    sqlite




def main():
    sql = sqlite3.connect("data.db")
    logging.config.fileConfig("logging.ini")
    config = xbxc.Config("config.ini")
    builds = build(config)
    for b in builds:
        save_build(b.stats)




if __name__ == "__main__":
    main()
