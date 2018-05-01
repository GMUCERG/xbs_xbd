#! /usr/bin/env python3

import sys
sys.path.append('./python')

import logging.config

import xbx.config as xbxc
import xbx.run as xbxr
import xbx.run_op as xbxro
import xbx.util as xbxu
import xbx.database as xbxdb

import csv
CONFIG_PATH="config.ini"



def main():
    logging.config.fileConfig("logging.ini", disable_existing_loggers=False)
    xbxdb.init(xbxu.get_db_path(CONFIG_PATH))
    s = xbxdb.scoped_session()
    q = s.query(xbxro.CryptoAeadRun)

    with open ("data.csv","w") as csvfile:
        w = csv.writer(csvfile)
        w.writerow([
            'algorithm',
            'implementation',
            'compiler version',
            'compiler command',
            'message_len (B)',
            'ad_len (B)',
            'tp (cycles/B)',
            'cycles',
            'rom',
            'ram',
            'max power (W)',
            'average power (W)',
            'energy(J)'
            'mode',
        ])

        for i in q:
            print(i)
            w.writerow([
                i.build_exec.build.implementation.path,
                i.build_exec.build.implementation.primitive_name,
                i.build_exec.build.compiler.cc_version,
                i.build_exec.build.compiler.cc,
                i.plaintext_len,
                i.assoc_data_len,
                i.measured_cycles/(i.plaintext_len + i.assoc_data_len),
                i.measured_cycles,
                i.build_exec.build.data + i.build_exec.build.text,
                i.build_exec.build.bss +i.build_exec.build.data +i.stack_usage,
                i.max_power,
                i.avg_power,
                i.total_energy,
                i.mode,
                i.timestamp.strftime("%Y-%m-%dT%H:%M:%SZ")
            ])



if __name__ == "__main__":
    main()
