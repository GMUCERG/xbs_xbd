#! /usr/bin/python3
import configparser
import distutils.util
import logging
import os
import re
import shlex
import shutil
import string
import subprocess
import sys
import hashlib

from sqlalchemy.schema import ForeignKeyConstraint, PrimaryKeyConstraint
from sqlalchemy import Column, ForeignKey, Integer, String, Text, Boolean, DateTime
from sqlalchemy.orm import relationship, reconstructor

from xbx.dirchecksum import dirchecksum
import xbx.database as xbxdb

from xbx.database import Base, unique_constructor, scoped_session
import xbx.util

_logger=logging.getLogger(__name__)

DEFAULT_CONF = os.path.join(os.path.dirname(__file__), "config.ini")

@unique_constructor(scoped_session, 
        lambda **kwargs: kwargs['hash'], 
        lambda query, **kwargs: query.filter(Platform.hash == kwargs['hash']))
class Platform(Base):# {{{
    __tablename__  = "platform"

    hash           = Column(String)
    name           = Column(String)
    clock_hz       = Column(Integer)
    pagesize       = Column(Integer)
    path           = Column(String)
    tmpl_path      = Column(String)

    compilers = relationship(
        "Compiler", 
        backref="platform",
    )

    __table_args__ = (
        PrimaryKeyConstraint("hash"),
    )
    @property
    def valid_hash(self):
        """Verifies if hash still valid"""
        hash = dirchecksum(self.path)
        return hash == self.hash
    # @reconstructor
    # def load_init(self):
    #     """Verifies if hash still valid and saves new result in DB"""
    #     self.valid_hash = self.validate_hash()
    #     scoped_session().add(self)

    # def __init__(self, **kwargs):
    #     super().__init__(**kwargs)
    #     self.valid_hash = self.validate_hash()
    #     
    # def validate_hash(self):
    #     """Indicates if current existing directory validates with stored hash"""
    #     hash = dirchecksum(self.path)
    #     valid_hash = None
    #     if self.hash != hash:
    #         valid_hash = False
    #     else:
    #         valid_hash = True
    #     return valid_hash
# }}}


class Compiler(Base):# {{{
    __tablename__ = "compiler"

    platform_hash = Column(String)
    idx = Column(Integer)

    cc_version = Column(String)
    cxx_version = Column(String)
    cc_version_full = Column(String)

    cxx_version_full = Column(String)
    cc = Column(String)
    cxx = Column(String)
    __table_args__ = (
        PrimaryKeyConstraint("platform_hash", "idx", ),
        ForeignKeyConstraint(["platform_hash"], ["platform.hash"]),
    )
    pass # For indentation
# }}}

@unique_constructor(scoped_session, 
        lambda **kwargs: kwargs['name'], 
        lambda query, **kwargs: query.filter(Operation.name == kwargs['name']))
class Operation(Base):# {{{
    __tablename__ = "operation"

    name          = Column(String)
    # String from OPERATIONS file
    operation_str = Column(String)

    primitives = relationship( "Primitive", backref="operation")

    __table_args__ = (
        PrimaryKeyConstraint("name"),
    )

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.parse_opstring()

    @reconstructor 
    def parse_opstring(self):
        match            = re.match(r'crypto_(\w+) ([:_\w]+) (.*)$', self.operation_str)
        self.macros      = match.group(2).split(':')
        self.macros     += '',
        self.prototypes  = match.group(3).split(':')
# }}}


@unique_constructor(scoped_session, 
        lambda **kwargs: kwargs['name'], 
        lambda query, **kwargs: query.filter(Primitive.name == kwargs['name']))
class Primitive(Base):# {{{
    __tablename__  = "primitive"

    name           = Column(String)
    operation_name = Column(String)
    checksumsmall  = Column(String)
    checksumlarge  = Column(String)
    path           = Column(String)

    implementations = relationship(
        "Implementation", 
        backref="primitive",
        #collection_class=set
    )

    __table_args__ = (
        PrimaryKeyConstraint("operation_name", "name"),
        ForeignKeyConstraint(
            ["operation_name"],
            ["operation.name"]
        ),
    )
# }}}


@unique_constructor(scoped_session, 
        lambda **kwargs: kwargs['hash'], 
        lambda query, **kwargs: query.filter(Implementation.hash == kwargs['hash']))
class Implementation(Base):# {{{
    __tablename__  = "implementation"
    hash           = Column(String)
    name           = Column(String)
    operation_name = Column(String)
    primitive_name = Column(String)
    path           = Column(String)

    __table_args__ = (
        PrimaryKeyConstraint("hash"),
        ForeignKeyConstraint(
            ["primitive_name", "operation_name"],
            ["primitive.name", "primitive.operation_name"],
        ),
    )

    @property
    def valid_hash(self):
        """Verifies if hash still valid"""
        hash = dirchecksum(self.path)
        return hash == self.hash
    # @reconstructor
    # def load_init(self):
    #     """Verifies if hash still valid and saves new result in DB"""
    #     self.valid_hash = self.validate_hash()
    #     scoped_session().add(self)

    # def __init__(self, **kwargs):
    #     super().__init__(**kwargs)
    #     self.valid_hash = self.validate_hash()
    #     
    # def validate_hash(self):
    #     """Indicates if current existing directory validates with stored hash"""
    #     hash = dirchecksum(self.path)
    #     valid_hash = None
    #     if self.hash != hash:
    #         valid_hash = False
    #     else:
    #         valid_hash = True
    #     return valid_hash
# }}}


@unique_constructor(scoped_session, 
        lambda config_path, **kwargs: hash_config(config_path), 
        lambda query, config_path, **kwargs: query.filter(Config.hash == hash_config(config_path)))
class Config(Base):# {{{
    """Configuration for running benchmarks on a single operation on a single platform"""

    __tablename__    = "config"

    hash             = Column(String)

    config_path      = Column(String)

    platforms_path   = Column(String)
    algopack_path    = Column(String)
    embedded_path    = Column(String)
    work_path        = Column(String)
    data_path        = Column(String)

    xbh_addr         = Column(String)
    xbh_port         = Column(Integer)

    platform_hash    = Column(String)
    operation_name   = Column(String)

    drift_measurements = Column(Integer)
    checksum_tests   = Column(Integer)
    operation_params = Column(xbxdb.JSONEncodedDict)
    xbh_timeout      = Column(Integer)
    exec_runs        = Column(Integer)

    blacklist        = Column(xbxdb.JSONEncodedDict)
    whitelist        = Column(xbxdb.JSONEncodedDict)

    one_compiler     = Column(Boolean)
    parallel_build   = Column(Boolean)

    platform         = relationship( "Platform", uselist=False)
    operation        = relationship( "Operation", uselist=False)

    __table_args__   = (
        PrimaryKeyConstraint("hash" ),
        ForeignKeyConstraint(["platform_hash"], ["platform.hash"]),
        ForeignKeyConstraint(["operation_name"], ["operation.name"]),
    )


    def __init__(self, config_path, **kwargs):
        super().__init__(**kwargs)
        _logger.debug("Loading configuration")
        config = configparser.ConfigParser()
        config.read(DEFAULT_CONF)
        config.read(config_path)
        self.config_path = config_path

        self.hash = hash_config(config_path)

        ## Basic path configuration
        self.platforms_path = config.get('paths','platforms')
        self.algopack_path  = config.get('paths','algopacks')
        self.embedded_path  = config.get('paths','embedded')
        self.work_path      = config.get('paths','work')
        self.data_path      = config.get('paths','data')

        self.one_compiler = bool(distutils.util.strtobool(
            config.get('build', 'one_compiler')))
        self.parallel_build = bool(distutils.util.strtobool(
            config.get('build', 'parallel_build')))

        # XBH address
        self.xbh_addr = config.get('xbh', 'address')
        self.xbh_port = config.get('xbh', 'port')


        # Platform
        self.platform = Config.__enum_platform(
                config.get('hardware','platform'),
                self.platforms_path)

        # Operation
        name                    = config.get('algorithm','operation')
        op_filename             = config.get('paths','operations')
        self.operation          = Config.__enum_operation(name, op_filename)

        # Runtime parameters
        self.drift_measurements = config.get('run','drift_measurements')
        self.checksum_tests     = config.get('run','checksum_tests')
        self.xbh_timeout        = config.get('run','xbh_timeout')
        self.exec_runs          = config.get('run','exec_runs')

        # Parameters
        self.operation_params   = []
        op_params               = config.get('run',self.operation.name+"_parameters").split("\n")
        for line in op_params:
            row = line.split(',')
            row = list(map(lambda val: int(val), row))
            self.operation_params += [row]


        # TODO Read platform blacklists
        # Update platform blacklist sha256sums

        self.blacklist = []
        self.whitelist = []

        if config.has_option('implementation', 'blacklist') and config.get('implementation','blacklist'):
            self.blacklist = config.get('implementation','blacklist').split("\n")

        if config.has_option('implementation', 'whitelist') and config.get('implementation','whitelist'):
            self.whitelist = config.get('implementation','whitelist').split("\n")

        primitives = config.get('algorithm','primitives').split("\n")
        self.operation.primitives = Config.__enum_prim_impls(
            self.operation, 
            primitives, 
            self.blacklist,
            self.whitelist, 
            self.algopack_path
        )



        

    @staticmethod
    def __enum_platform(name, platforms_path):# {{{
        """Enumerate platform settings, given path to platform directory"""
        _logger.debug("Enumerating platforms")

        ## Platform configuration
        path = os.path.join(platforms_path, name)

        config = configparser.ConfigParser()
        config.read(os.path.join(path, "settings.ini"))
        tmpl_path = None
        if config.has_option('platform_settings', 'templatePlatform'):
            tmpl_path = os.path.join(
                    self.platforms_path,
                    config.get('platform_settings','templatePlatform'))

        clock_hz = config.getint('platform_settings','cyclespersecond')
        pagesize = config.getint('platform_settings','pagesize')

        compilers = Config.__enum_compilers(path, tmpl_path)

        
        hash = dirchecksum(path)
        if tmpl_path:
            h = hashlib.sha256()
            h.update(hash)
            h.update(dirchecksum(tmpl_path))
            hash = h.sha256()

        return Platform(
                hash=hash,
                name=name, 
                path=path, 
                tmpl_path=tmpl_path, 
                clock_hz=clock_hz,
                pagesize=pagesize, 
                compilers=compilers
            )

    # }}}

    @staticmethod
    def __enum_compilers(platform_path, tmpl_path):# {{{
        _logger.debug("Enumerating compilers")
        cc_list = []
        cxx_list = []
        compilers = []
        env = os.environ.copy()
        env.update({'templatePlatformDir': tmpl_path if tmpl_path else ''})
        c_compilers = os.path.join(platform_path,'c_compilers')
        cxx_compilers = os.path.join(platform_path,'cxx_compilers')
        if os.path.isfile(c_compilers):
            stdout = subprocess.check_output([c_compilers], env=env)
            cc_list += stdout.decode().splitlines()
        if os.path.isfile(cxx_compilers):
            stdout= subprocess.check_output([cxx_compilers], env=env)
            cxx_list += stdout.decode().splitlines()

        if (len(cc_list) != 0 and
                len(cxx_list) != 0 and
                len(cc_list) != len(cxx_list)):
            raise ValueError("c_compilers and cxx_compilers "+
                    "must have equal length output")
        elif len(cc_list) == 0 and len(cxx_list) == 0:
            raise ValueError("At least one of c_compilers or "+
                    "cxx_compilers must have nonzero output")

        elif len(cxx_list) == 0:
            cxx_list = ['']*len(cc_list)
        elif len(cc_list) == 0:
            cc_list = ['']*len(cxx_list)

        def get_version(compiler):
            cmd = compiler.partition(" ")[0],
            cmd += '-v',
            version_full = subprocess.check_output(
                    cmd, stderr=subprocess.STDOUT).decode().strip()
            version = version_full.splitlines()[-1]
            return version, version_full


        for i in range(0, len(cc_list)):
            cc_version, cc_version_full, cxx_version, cxx_version_full = ('','','','')
            if cc_list[i]:
                cc_version, cc_version_full = get_version(cc_list[i])
            if cxx_list[i]:
                cxx_version, cxx_version_full = get_version(cxx_list[i])

            compilers += Compiler(
                idx=i,
                cc=cc_list[i], 
                cxx=cxx_list[i],
                cc_version=cc_version,
                cxx_version=cxx_version,
                cc_version_full=cc_version_full,
                cxx_version_full=cxx_version_full
            ),

        
        return compilers

    # }}}

    @staticmethod
    def __enum_prim_impls(operation, primitive_names, blacklist, whitelist, algopack_path):# {{{
        """Get primitive implementations 
        
        Blacklist has priority: impls = whitelist & ~blacklist

        Parameters:
            operation       Instance of Operation
            primitive_names List of primitive names
            blacklist       List of regexes to match of implementations to
                            remove
            whitelist       List of implementations to consider. """
            

        _logger.debug("Enumerating primitives and implementations")
        primitives = []

        # Get operation path
        op_path = os.path.join(algopack_path, "crypto_"+operation.name)

        # Get list of primitive directories
        primdirs = [d for d in os.listdir(op_path) if os.path.isdir(os.path.join(op_path, d))]

        # Get prim directories intersecting w/ primitive list
        for name in set(primitive_names) & set(primdirs):
            # Insert primitive as key into primitives
            # path and implementations as value
            path = os.path.join(op_path,name)
            operation = operation
            checksumfile = os.path.join(path, "checksumsmall")
            checksumsmall = ""
            with open(checksumfile) as f:
                checksumsmall = f.readline().strip()

            p = Primitive(
                    name=name, 
                    path=path,
                    checksumsmall=checksumsmall
                )

            primitives += p,

        # Get whitelist  ^blacklist
        for p in primitives:
            all_impls = {}

            # Find all directories w/ api.h
            walk =  os.walk(p.path)
            for i in walk:
                if "api.h" in i[2]:
                    path = os.path.relpath(i[0], p.path)
                    name = path.translate(str.maketrans("./-","___"))
                    all_impls[name] = path

            impl_set = set(all_impls.keys())

            for i in all_impls.keys():
                for j in blacklist:
                    if re.match(j,i):
                        impl_set.remove(i)

            if whitelist:
                impl_set &= set(whitelist);

            for name in impl_set:
                path = os.path.join(p.path,all_impls[name])
                checksum = dirchecksum(path)
                # Don't have to add to p.implementations, as Implementation sets
                # Primitive as parent, which inserts into list
                Implementation(
                    hash=checksum,
                    name=name, 
                    path=path,
                    primitive=p
                )

        return primitives

    # }}}

    @staticmethod
    def __enum_operation(name, filename):# {{{
        """Generate operation"""
        _logger.debug("Enumerating operations")

        with open(filename) as f:
            for l in f:
                match = re.match(r'crypto_(\w+) ([:_\w]+) (.*)$', l)
                if match.group(1) == name:
                    return Operation(name=name, operation_str=l.strip())
    # }}}

# }}}
config_hash_cache = {}

def hash_config(config_path):
    config_hash = xbx.util.sha256_file(config_path)

    if config_hash in config_hash_cache:
        return config_hash_cache[config_hash]

    config = configparser.ConfigParser()
    config.read(DEFAULT_CONF)
    config.read(config_path)

    file_paths = (
        DEFAULT_CONF,
        config.get('paths','operations')
    )

    dir_paths = ( 
        config.get('paths','platforms'),
        config.get('paths','algopacks'),
        config.get('paths','embedded'),
        #config.get('paths','work')
    )

    dir_hashes = list(map(lambda x: dirchecksum(x), dir_paths))
    file_hashes = list(map(lambda x: xbx.util.sha256_file(x), file_paths))

    h = hashlib.sha256()
    for f in (dir_hashes+file_hashes):
        h.update(f.encode())

    h.update(config_hash.encode())

    config_hash_cache[config_hash] = h.hexdigest()

    return config_hash_cache[config_hash]
