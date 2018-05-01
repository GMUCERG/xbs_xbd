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
import pandas as pd

CONFIG_PATH="config.ini"


def main():
    logging.config.fileConfig("logging.ini", disable_existing_loggers=False)
    xbxdb.init(xbxu.get_db_path(CONFIG_PATH))
    s = xbxdb.scoped_session()
    rs = s.query(xbxr.RunSession).order_by(xbxr.RunSession.id.desc()).first()


    # q is filtered for everything in last runsession, only encryption results
    q = s.query(xbxro.CryptoAeadRun).join(xbxr.BuildExec).join(xbxr.RunSession).filter(xbxr.RunSession.id == rs.id).filter(xbxro.CryptoAeadRun.mode == 'enc')

    # use pandas to make columns, then group into one table (dataframe)

    ram = pd.Series([i.build_exec.build.bss +i.build_exec.build.data +i.stack_usage for i in q])
    rom = pd.Series([i.build_exec.build.data +i.build_exec.build.text for i in q])

    tp = pd.Series([i.measured_cycles/(i.plaintext_len + i.assoc_data_len) for i in q ])

    enrgy = pd.Series([i.total_energy for i in q ])
    avg_pwr = pd.Series([i.avg_power for i in q ])
    max_pwr = pd.Series([i.max_power for i in q ])
    length = pd.Series([i.plaintext_len+i.assoc_data_len for i in q ])

    df = pd.DataFrame({'length':length,'ram':ram,'rom':rom,'tp':tp,'enrgy':enrgy,'avg_pwr':avg_pwr,'max_pwr':max_pwr}).sort_values(by='length')

    print(df)



    ax1 = plt.subplot(311)
    ax1.set_ylabel("memory (B)")
    ax1.set_xlabel("length (M+AD)")
    ax1.plot(df['length'], df['ram'], '-', color='red')
    ax1.plot(df['length'], df['rom'], '-',color='orange' )
    ax1.legend(['ram', 'rom'])

    ax2_1 = plt.subplot(312)
    ax2_1.plot(df['length'], df['avg_pwr'], '-', color='yellow')
    ax2_1.plot(df['length'], df['max_pwr'], '-', color='green')
    ax2_1.set_ylabel("power (W)")
    ax2_1.set_xlabel("length (M+AD)")

    ax2_2 = ax2_1.twinx()
    ax2_2.plot(df['length'],df['enrgy'], '-', color="blue")
    ax2_2.set_ylabel("energy (J)")
    ax2_1.legend(['avg_pwr', 'max_pwr'], loc=6)
    ax2_2.legend(['energy'], loc=7)

    ax3 = plt.subplot(313)
    ax3.plot(df['length'],df['tp'], '-')
    ax3.set_xlabel("length (M+AD)")
    ax3.set_ylabel("Throughput (Cycles/byte")
    ax3.set_yscale("log", nonposy='clip')
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
