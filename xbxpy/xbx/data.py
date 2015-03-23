import logging
import datetime
import sqlite3
import hashlib
import socket
import os
import yaml

import xbx.build
import xbx.config
import xbx.util

_logger = logging.getLogger(__name__)

class Database:
    def __init__(self, config):
        self.config = config
        if not os.path.isfile(config.data_path):
            _logger.info("Database doesn't exist, initializing")

            self.sql_conn = sqlite3.connect(config.data_path)
            self.initdb()
        else:
            self.sql_conn = sqlite3.connect(config.data_path)
        self.save_metadata()
        self.config_hash = self.save_config()
        self.commit()

    # xbx.build.Build # {{{
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
                 build.parallel_make,

                 build.text,
                 build.data,
                 build.bss,
                 
                 build.rebuilt,

                 build.timestamp,
                 build.hex_checksum,
                 build_session.session_id,

                 build.checksumsmall_result,
                 build.checksumlarge_result,
                 build.test_ok)
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
            "parallel_make," 

            "text," 
            "data," 
            "bss," 

            "rebuilt,"

            "timestamp," 
            "hex_checksum," 
            "build_session," 
            
            "checksumsmall_result,"
            "checksumlarge_result," 
            "test_ok"
            ") values ("
            "?, ?, ?, ?, ?, ?,  ?, ?, ?,  ?, ?, ?,  ?,  ?, ?, ?,  ?, ?, ?)"),
            data)

    def load_builds(self, build_session, config):
        """Loads all builds from build_session"""
        data = build_session,

        builds = []

        cursor = self.sql_conn.cursor()
        cursor.execute(("select "
            "primitive," 
            "implementation," 
            "compiler_idx," 

            "exe_path," 
            "hex_path," 
            "parallel_make," 

            "text," 
            "data," 
            "bss," 

            "rebuilt,"

            "timestamp," 
            "hex_checksum," 
            
            "checksumsmall_result,"
            "checksumlarge_result" 
            " from build where build_session=?"), data)

        for row in iter(cursor.fetchone, None):
            impl = config.operation.primitives[row[0]].impls[row[1]]
            idx = row[2]

            build = xbx.build.Build(config, idx, impl)

            (_, _, _,
                    build.exe_path,
                    build.hex_path,
                    build.parallel_make,

                    build.text,
                    build.data,
                    build.bss,

                    build.rebuilt,

                    build.timestamp,
                    build.hex_checksum,

                    build.checksumsmall,
                    build.checksumlarge) = row
            builds += build,

        return builds

    # }}}

    # xbx.build.BuildSession# {{{
    def save_buildsession(self, build_session):

        data = (build_session.timestamp, 
                build_session.host, 
                build_session.xbx_version,
                build_session.parallel,
                self.config_hash)
        cursor = self.sql_conn.cursor()

        cursor.execute("insert into build_session("
                "timestamp, host, xbx_version, parallel, config) "
                "values (?, ?, ?,  ?, ?)", data)
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

    def load_buildsession(self, bs_id):
        cursor = self.sql_conn.cursor()
        cursor.execute("select "
                "id, timestamp, host, xbx_version, parallel, config "
                "from build_session where id=?", (bs_id,))

        row = cursor.fetchone()

        if row == None:
            raise ValueError("BuildSession w/ id "+bs_id+" does not exist")

        
        config = self.load_config(row[-1]) 

        bs = xbx.build.BuildSession(config)
        (bs.session_id, bs.timestamp, bs.host, bs.xbx_version, 
                bs.parallel, _) = row


        bs.builds = self.load_builds(bs_id, config)
        return bs



        



    def get_newest_buildsession(self):
        cursor = self.sql_conn.cursor()
        cursor.execute(
                "select id from build_session "
                "order by timestamp desc limit 1")
        row = cursor.fetchone()
        if row == None:
            raise ValueError(
                    "No existing BuildSession found. Run compile first")

        rowid, = row
        # if config != hash_obj(self.config):
        #     raise ValueError(
        #             "Config does not match that of last "
        #             "build. Run compile again")

        return self.load_buildsession(rowid)
    # }}}

    # xbx.run.Run #{{{ 
    # }}}

    # xbx.build.RunSession# {{{
    def save_runsession(self, run_session):

        data = (datetime.datetime.now(), 
                socket.gethostname(), 
                run_session.xbx_version,
                run_session.buildsession.session_id,
                self.config_hash)
        cursor = self.sql_conn.cursor()

        cursor.execute("insert into run_session("
                "timestamp, host, xbx_version, build_session, config) "
                "values (?, ?, ?, ?, ?)", data)

        return cursor.lastrowid

    # }}}

    # xbx.config.Config# {{{
    def save_config(self):
        """Saves dump of config, and loads operation and primitive info
        into database"""

        c_hash = xbx.util.hash_obj(self.config)
        dump = None

        dump = yaml.dump(self.config)

        data = (c_hash , dump)
        
        cursor = self.sql_conn.cursor()
        cursor.execute(
                "insert or ignore into config(hash, dump) values (?, ?)", data)

        return c_hash

    def load_config(self, c_hash):
        cursor = self.sql_conn.cursor()
        cursor.execute(
                "select dump from  config where hash=?", (c_hash,))
        
        row = cursor.fetchone()
        if row == None:
            raise ValueError(
                    "No config with hash "+c_hash+" found.")

        import yaml
        return yaml.load(row[0])

       
 

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

        for p in self.config.operation.primitives.values():
            data = (p.operation.name,
                    p.name,
                    p.checksumsmall)

            cursor.execute(
                    op + " into primitive(operation, name, " 
                    "checksumsmall) values (?, ?, ?)", data)
    # }}}
    
    def initdb(self):
        schema = os.path.join(os.path.dirname(__file__), "schema.sql")
        with open(schema) as f:
            cursor = self.sql_conn.cursor()
            cursor.executescript(f.read())
            self.sql_conn.commit()



    def commit(self):
        self.sql_conn.commit()
