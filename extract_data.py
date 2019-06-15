#! /usr/bin/env python3
# Updated to include hash, kem, and sign support

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

def get_op_details(name):
    if name == "hash":
        return (xbxro.CryptoHashRun,
                [
                    'algorithm',
                    'implementation',
                    'compiler version',
                    'compiler command',
                    'message_len (B)',
                    'tp (cycles/B)',
                    'cycles',
                    'rom',
                    'ram',
                    'max power (W)',
                    'average power (W)',
                    'energy(J)',
					'timestamp'
                ])
    elif name == "aead":
        return (xbxro.CryptoAeadRun,
                [
                    'algorithm',
                    'mode',
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
                    'energy(J)',
					'timestamp'
                ])
    elif name == "kem":
        return (xbxro.CryptoKemRun,
                [
                    'algorithm',
                    'mode',
                    'implementation',
                    'compiler version',
                    'compiler command',
                    'secret key len',
					'public key len',
                    'cycles',
                    'rom',
                    'ram',
                    'max power (W)',
                    'average power (W)',
                    'energy(J)',
					'timestamp'
                ])
    elif name == "sign":
        return (xbxro.CryptoSignRun,
                [
                    'algorithm',
                    'mode',
                    'implementation',
                    'compiler version',
                    'compiler command',
					'secret key len',
					'public key len',
                    'message_len (B)',
                    'cycles',
                    'rom',
                    'ram',
                    'max power (W)',
                    'average power (W)',
                    'energy(J)',
					'timestamp'
                ])
    else:
        raise ValueError("Operation '{}' unknown".format(name))

def get_op_data(name, i):
    if name == "hash":
        return [
                    i.build_exec.build.implementation.primitive_name,
                    i.build_exec.build.implementation.path,
                    i.build_exec.build.compiler.cc_version,
                    i.build_exec.build.compiler.cc,
                    i.msg_len,
                    i.measured_cycles/i.msg_len,
                    i.measured_cycles,
                    i.build_exec.build.data + i.build_exec.build.text,
                    i.build_exec.build.bss + i.build_exec.build.data + i.stack_usage,
                    i.max_power,
                    i.avg_power,
                    i.total_energy,
                    i.timestamp.strftime("%Y-%m-%dT%H:%M:%SZ")]
    elif name == "aead":
        return [
                    i.build_exec.build.implementation.primitive_name,
                    i.mode,
                    i.build_exec.build.implementation.path,
                    i.build_exec.build.compiler.cc_version,
                    i.build_exec.build.compiler.cc,
                    i.plaintext_len,
                    i.assoc_data_len,
                    i.measured_cycles/(i.plaintext_len + i.assoc_data_len),
                    i.measured_cycles,
                    i.build_exec.build.data + i.build_exec.build.text,
                    i.build_exec.build.bss + i.build_exec.build.data + i.stack_usage,
                    i.max_power,
                    i.avg_power,
                    i.total_energy,
                    i.timestamp.strftime("%Y-%m-%dT%H:%M:%SZ")]
    elif name == "kem":
        return [
                    i.build_exec.build.implementation.primitive_name,
                    i.mode,
                    i.build_exec.build.implementation.path,
                    i.build_exec.build.compiler.cc_version,
                    i.build_exec.build.compiler.cc,
					i.secret_key_len,
					i.public_key_len,
                    i.measured_cycles,
                    i.build_exec.build.data + i.build_exec.build.text,
                    i.build_exec.build.bss + i.build_exec.build.data + i.stack_usage,
                    i.max_power,
                    i.avg_power,
                    i.total_energy,
                    i.timestamp.strftime("%Y-%m-%dT%H:%M:%SZ")]
    elif name == "sign":
        return [
                    i.build_exec.build.implementation.primitive_name,
                    i.mode,
                    i.build_exec.build.implementation.path,
                    i.build_exec.build.compiler.cc_version,
                    i.build_exec.build.compiler.cc,
					i.secret_key_len,
					i.public_key_len,
                    i.msg_len,
                    i.measured_cycles,
                    i.build_exec.build.data + i.build_exec.build.text,
                    i.build_exec.build.bss + i.build_exec.build.data + i.stack_usage,
                    i.max_power,
                    i.avg_power,
                    i.total_energy,
                    i.timestamp.strftime("%Y-%m-%dT%H:%M:%SZ")]
    else:
        raise ValueError("Operation '{}' unknown".format(name))

def main(name):
    logging.config.fileConfig("logging.ini", disable_existing_loggers=False)
    xbxdb.init(xbxu.get_db_path(CONFIG_PATH))
    s = xbxdb.scoped_session()
    t,c = get_op_details(name)
    q = s.query(t)

    with open ("data.csv","w") as csvfile:
        w = csv.writer(csvfile)
        w.writerow(c)

        for i in q:
            print(i)
            w.writerow(get_op_data(name, i))

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: {} [hash|aead|kem|sign]".format(sys.argv[0]))
        sys.exit(1)
    main(sys.argv[1])
