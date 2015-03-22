import datetime
import sqlite3
import hashlib
import socket

import xbx.build
import xbx.config
import xbx.util

class Database:
    def __init__(self, config):
        self.config = config
        self.sql_conn = sqlite3.connect(config.data_path)
        self.save_metadata()
        self.config_hash = self.save_config()

    def save_build(self, build, build_session):
        """Saves build stats using given sql cursor"""

        data = ( build.platform.name,
                 build.operation.name,
                 build.primitive.name,
                 build.implementation.name,
                 build.impl_checksum,
                 build.compiler_idx,

                 build.exe_path,
                 build.hex_path,
                 build.parallel,

                 build.text,
                 build.data,
                 build.bss,

                 build.timestamp,
                 build.hex_checksum,
                 build_session.session_id)
        cursor = self.sql_conn.cursor()
        cursor.execute(("insert into build ("
            "platform," 
            "operation," 
            "primitive," 
            "implementation," 
            "impl_checksum," 
            "compiler_idx," 

            "exe_path," 
            "hex_path," 
            "parallel," 

            "text," 
            "data," 
            "bss," 

            "timestamp," 
            "hex_checksum," 
            "build_session" 
            ") values ("
            "?, ?, ?, ?, ?, ?,  ?, ?, ?,  ?, ?, ?,  ?, ?, ?)"), data)

    def save_buildsession(self, build_session):

        data = (datetime.datetime.now(), 
                socket.gethostname(), 
                build_session.xbx_version,
                self.config_hash)
        cursor = self.sql_conn.cursor()

        cursor.execute("insert into build_session("
                "timestamp, host, xbx_version, config) values (?, ?, ?, ?)", data)
        build_session_id = cursor.lastrowid



        query = ("insert into compiler("
            "build_session,"
            "platform,"
            "idx,"
            "cc_version,"
            "cxx_version,"
            "cc_version_full,"
            "cxx_version_full,"
            "cc,"
            "cxx"
            ") values ("
            "?, ?, ?, ?, ?, ?, ?, ?, ?)")
        data = []
        for i in range(len(build_session.config.platform.compilers)):
            c = build_session.config.platform.compilers[i]
            data += (build_session_id,
                    build_session.config.platform.name,
                    i,
                    c.cc_version,
                    c.cxx_version,
                    c.cc_version_full,
                    c.cxx_version_full,
                    c.cc,
                    c.cxx),
        cursor.executemany(query, data)
        return build_session_id


    def save_config(self):
        """Saves dump of config, and loads operation and primitive info
        into database"""

        c_hash = xbx.util.hash_obj(self.config)
        dump = None

        if self.config.dump_config:
            import yaml
            dump = yaml.dump(self.config)

        data = (c_hash , dump)
        
        cursor = self.sql_conn.cursor()
        cursor.execute(
                "insert or ignore into config(hash, dump) values (?, ?)", data)

        return c_hash
       
 

    def save_metadata(self, replace=False):
        op = "insert or ignore"
        if replace:
            op = "replace"

        cursor = self.sql_conn.cursor()

        data = (self.config.platform.name,
                self.config.platform.clock_hz,
                self.config.platform.pagesize)

        cursor.execute(op + " into platform(name, clock_hz, pagesize)"
                " values (?, ?, ?)", data)

        data = (self.config.operation.name,)
        cursor.execute(op + " into operation(name) values (?)", data)

        for p in self.config.primitives:
            data = (p.operation.name,
                    p.name,
                    p.checksumsmall)

            cursor.execute(
                    op + " into primitive(operation, name, " 
                    "checksumsmall) values (?, ?, ?)", data)
   
    def commit(self):
        self.sql_conn.commit()
