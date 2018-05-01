#! /usr/bin/env python3

import sys
sys.path.append('./python')

import logging.config

import xbx.config as xbxc
import xbx.build as xbxb
import xbx.run as xbxr
import xbx.run_op as xbxro
import xbx.util as xbxu
import xbx.database as xbxdb

import matplotlib.pyplot as plt

CONFIG_PATH="config.ini"


def main():
    logging.config.fileConfig("logging.ini", disable_existing_loggers=False)
    xbxdb.init(xbxu.get_db_path(CONFIG_PATH))
    s = xbxdb.scoped_session()
    rs = s.query(xbxr.RunSession).order_by(xbxr.RunSession.id.desc()).first()


    # q is filtered for everything in last runsession
    q = s.query(xbxro.CryptoAeadRun).join(xbxr.BuildExec).join(xbxr.RunSession).filter(xbxr.RunSession.id == rs.id).filter(xbxro.CryptoAeadRun.mode == 'enc')

    ram = [i.build_exec.build.bss +i.build_exec.build.data +i.stack_usage for i in q]
    rom = [i.build_exec.build.data +i.build_exec.build.text for i in q]

    tp = [i.measured_cycles/(i.plaintext_len + i.assoc_data_len) for i in q ]

    enrgy = [i.total_energy for i in q ]
    avg_pwr = [i.avg_power for i in q ]
    max_pwr = [i.max_power for i in q ]



    length = [i.plaintext_len+i.assoc_data_len for i in q ]

    ax1 = plt.subplot(311)
    ax1.set_ylabel("memory (B)")
    ax1.set_xlabel("length (M+AD)")
    ax1.plot(length, ram, '^', color='red')
    ax1.plot(length,rom, 'o',color='orange' )
    ax1.legend(['ram', 'rom'])

    ax2_1 = plt.subplot(312)
    ax2_1.plot(length,avg_pwr, '^', color='yellow')
    ax2_1.plot(length, max_pwr, 'o', color='green')
    ax2_1.set_ylabel("power (W)")
    ax2_1.set_xlabel("length (M+AD)")

    ax2_2 = ax2_1.twinx()
    ax2_2.plot(length,enrgy, 'b+')
    ax2_2.set_ylabel("energy (J)")
    ax2_1.legend(['avg_pwr', 'max_pwr'], loc=6)
    ax2_2.legend(['energy'], loc=7)

    ax3 = plt.subplot(313)
    ax3.plot(length,tp, '^')
    ax3.set_xlabel("length (M+AD)")
    ax3.set_ylabel("Throughput (Cycles/byte")
    plt.show()

    
    


#    for i in q:
#        print(i)
#        w.writerow([
#            i.build_exec.build.implementation.path,
#            i.build_exec.build.implementation.primitive_name,
#            i.build_exec.build.compiler.cc_version,
#            i.build_exec.build.compiler.cc,
#            i.plaintext_len,
#            i.assoc_data_len,
#            i.measured_cycles/(i.plaintext_len + i.assoc_data_len),
#            i.measured_cycles,
#            i.build_exec.build.data + i.build_exec.build.text,
#            i.build_exec.build.bss +i.build_exec.build.data +i.stack_usage,
#            i.max_power,
#            i.avg_power,
#            i.total_energy,
#            i.mode,
#            i.timestamp.strftime("%Y-%m-%dT%H:%M:%SZ")
#        ])



if __name__ == "__main__":
    main()
