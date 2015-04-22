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
from sqlalchemy import Table, Column, ForeignKey, Integer, String, Text, Boolean, DateTime
from sqlalchemy.orm import relationship, reconstructor
from sqlalchemy.orm.exc import NoResultFound

from xbx.dirchecksum import dirchecksum
import xbx.database as xbxdb

from xbx.database import Base, unique_constructor, scoped_session, JSONEncodedDict
import xbx.util

_logger=logging.getLogger(__name__)

DEFAULT_CONF = os.path.join(os.path.dirname(__file__), "config.ini")

@unique_constructor(scoped_session,
        lambda **kwargs: kwargs['hash'],
        lambda query, **kwargs: query.filter(Platform.hash == kwargs['hash']))
class Platform(Base):
    __tablename__  = "platform"

    hash           = Column(String, nullable=False)
    name           = Column(String, nullable=False)
    clock_hz       = Column(Integer)
    pagesize       = Column(Integer)
    path           = Column(String) # Path relative to config.platforms_path
    tmpl_path      = Column(String)

    compilers = relationship(
        "Compiler",
        backref="platform",
    )

    __table_args__ = (
        PrimaryKeyConstraint("hash"),
    )
    def validate_hash(self, platforms_path):
        """Verifies if hash still valid"""
        hash = dirchecksum(os.path.join(platforms_path, self.path))
        return hash == self.hash

    @classmethod
    def from_path(cls, name, platforms_path):
        # TODO: Parse rest of settings.ini, eg memory available
        full_path = os.path.join(platforms_path, name)
        config = configparser.ConfigParser()
        config.read(os.path.join(full_path, "settings.ini"))
        tmpl_path = None
        if config.has_option('platform_settings', 'templatePlatform'):
            tmpl_path = os.path.join(
                    platforms_path,
                    config.get('platform_settings','templatePlatform'))

        clock_hz = config.getint('platform_settings','cyclespersecond')
        pagesize = config.getint('platform_settings','pagesize')

        compilers = cls.__enum_compilers(full_path, tmpl_path)


        hash = dirchecksum(full_path)
        tmpl_rel_path = None
        if tmpl_path:
            h = hashlib.sha256()
            h.update(hash)
            h.update(dirchecksum(tmpl_path))
            hash = h.sha256()
            if tmpl_path:
                tmpl_rel_path = os.path.relpath(tmpl_path, platforms_path)


        return cls(
            hash=hash,
            name=name,
            path=os.path.relpath(full_path, platforms_path),
            tmpl_path=tmpl_rel_path,
            clock_hz=clock_hz,
            pagesize=pagesize,
            compilers=compilers
        )

    @staticmethod
    def __enum_compilers(platform_path, tmpl_path):
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
            cmd = compiler.partition(" ")[0].strip(),
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
                cc=cc_list[i].strip(),
                cxx=cxx_list[i].strip(),
                cc_version=cc_version.strip(),
                cxx_version=cxx_version.strip(),
                cc_version_full=cc_version_full.strip(),
                cxx_version_full=cxx_version_full.strip()
            ),

        return compilers




class Compiler(Base):
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


@unique_constructor(scoped_session,
        lambda **kwargs: kwargs['name'],
        lambda query, **kwargs: query.filter(Operation.name == kwargs['name']))
class Operation(Base):
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
        match             = re.match(r'(\w+) ([:_\w]+) (.*)$', self.operation_str)

        # Use filter to ignore ''
        self.macro_names  = [x for x in match.group(2).split(':') if x]
        self.prototypes   = [x for x in match.group(3).split(':') if x]



@unique_constructor(scoped_session,
        lambda **kwargs: kwargs['name']+'_'+kwargs['operation'].name,
        lambda query, **kwargs: query.filter(Operation.name == kwargs['operation'].name,
                                             Primitive.name == kwargs['name']))
class Primitive(Base):
    __tablename__  = "primitive"

    name           = Column(String, nullable=False)
    operation_name = Column(String, nullable=False)
    checksumsmall  = Column(String)
    checksumbig    = Column(String)
    path           = Column(String)  #Path relative to algobase

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


# Join table for libsupercop dependencies
_impl_dep_join_table = Table('config_impl_dep_join', Base.metadata,
    Column('dependent_impl_hash', Integer,
           ForeignKey('implementation.hash', ondelete="CASCADE"),
           nullable=False, primary_key=True),
    Column('dependency_impl_hash', Integer,
           ForeignKey('implementation.hash', ondelete="CASCADE"),
           nullable=False, primary_key=True)
)

@unique_constructor(scoped_session,
        lambda **kwargs: kwargs['hash'],
        lambda query, **kwargs: query.filter(Implementation.hash == kwargs['hash']))
class Implementation(Base):
    __tablename__  = "implementation"
    hash           = Column(String, nullable=False)
    name           = Column(String, nullable=False)
    operation_name = Column(String, nullable=False)
    primitive_name = Column(String, nullable=False)
    path           = Column(String) #Path relative to algobase
    macros         = Column(JSONEncodedDict)

    dependencies   = relationship(
        "Implementation", backref="dependents",
        secondary=_impl_dep_join_table,
        primaryjoin=(_impl_dep_join_table.c.dependent_impl_hash == hash),
        secondaryjoin=(_impl_dep_join_table.c.dependency_impl_hash == hash)
    )
    __table_args__ = (
        PrimaryKeyConstraint("hash"),
        ForeignKeyConstraint(
            ["primitive_name", "operation_name"],
            ["primitive.name", "primitive.operation_name"],
        ),
    )


    def validate_hash(self, platforms_path):
        """Verifies if hash still valid"""
        hash = dirchecksum(os.path.join(platforms_path, self.path))
        return hash == self.hash


# Join table for primitves connected to config
_config_impl_join_table = Table(
    'config_implementation_join', Base.metadata,
    Column('implementation_hash', Integer,
           ForeignKey('implementation.hash', ondelete="CASCADE"),
           nullable=False, primary_key=True),
    Column('config_hash', Integer,
           ForeignKey('config.hash', ondelete="CASCADE"), nullable=False,
           primary_key=True)
)

_config_libsupercop_impl_join_table = Table(
    'config_libsupercop_impl_join', Base.metadata,
    Column('implementation_hash', Integer,
           ForeignKey('implementation.hash', ondelete="CASCADE"),
           nullable=False, primary_key=True),
    Column('config_hash', Integer,
           ForeignKey('config.hash', ondelete="CASCADE"), nullable=False,
           primary_key=True)
)


@unique_constructor(
    scoped_session,
    lambda config_path, **kwargs: hash_config(config_path),
    lambda query, config_path, **kwargs: query.filter(
        Config.hash == hash_config(config_path)
    )
)
class Config(Base):
    """Configuration for running benchmarks on a single operation on a single platform"""

    __tablename__    = "config"

    hash                  = Column(String, nullable=False)

    config_path           = Column(String, nullable=False)

    platforms_path        = Column(String, nullable=False)
    algopack_path         = Column(String, nullable=False)
    embedded_path         = Column(String, nullable=False)
    work_path             = Column(String, nullable=False)
    data_path             = Column(String, nullable=False)
    impl_conf_path        = Column(String, nullable=False)

    xbh_addr              = Column(String)
    xbh_port              = Column(Integer)

    platform_hash         = Column(String)
    operation_name        = Column(String)

    rerun                 = Column(Boolean)
    drift_measurements    = Column(Integer)
    checksum_tests        = Column(Integer)
    operation_params      = Column(xbxdb.JSONEncodedDict)
    xbh_timeout           = Column(Integer)
    exec_runs             = Column(Integer)

    blacklist             = Column(xbxdb.JSONEncodedDict)
    blacklist_regex       = Column(String)
    whitelist             = Column(xbxdb.JSONEncodedDict)
    enforce_bwlist_checksums = Column(Boolean)

    one_compiler          = Column(Boolean)
    parallel_build        = Column(Boolean)


    platform              = relationship("Platform", uselist=False)
    operation             = relationship("Operation", uselist=False)
    implementations       = relationship("Implementation",
                                         secondary=_config_impl_join_table)

    libsupercop_impls     = relationship("Implementation",
                                         secondary=_config_libsupercop_impl_join_table)


    __table_args__   = (
        PrimaryKeyConstraint("hash" ),
        ForeignKeyConstraint(["platform_hash"], ["platform.hash"]),
        ForeignKeyConstraint(["operation_name"], ["operation.name"]),
    )


    def __init__(self, config_path, **kwargs):
        super().__init__(**kwargs)
        _logger.debug("Loading configuration")
        config = configparser.ConfigParser()
        config.read_file(open(DEFAULT_CONF))
        config.read(config_path)
        self.config_path = config_path

        self.hash = hash_config(config_path)

        ## Basic path configuration
        self.platforms_path        = config.get('paths','platforms')
        self.algopack_path         = config.get('paths','algopacks')
        self.embedded_path         = config.get('paths','embedded')
        self.work_path             = config.get('paths','work')
        self.data_path             = config.get('paths','data')
        self.impl_conf_path = config.get('paths', 'impl_conf')

        self.one_compiler = config.getboolean('build', 'one_compiler')
        self.parallel_build = config.getboolean('build', 'parallel_build')

        # XBH address
        self.xbh_addr = config.get('xbh', 'address')
        self.xbh_port = config.get('xbh', 'port')


        # Platform
        self.platform = Platform.from_path(config.get('hardware','platform'),
                                           self.platforms_path)

        # Operation
        name                    = config.get('algorithm','operation')
        op_filename             = config.get('paths','operations')
        self.operation          = _enum_operation(name, op_filename)

        # Runtime parameters
        self.rerun              = config.getboolean('run', 'rerun')
        self.drift_measurements = config.getint('run','drift_measurements')
        self.checksum_tests     = config.getint('run','checksum_tests')
        self.xbh_timeout        = config.getint('run','xbh_timeout')
        self.exec_runs          = config.getint('run','exec_runs')

        # Parameters
        self.operation_params   = []
        op_params               = config.get('run',self.operation.name+
                                             "_parameters").split("\n")
        for line in op_params:
            if line:
                row = line.split(',')
                row = list(map(lambda val: int(val), row))
                self.operation_params += [row]


        primitives = config.get('algorithm','primitives').strip().split("\n")


        impl_conf = configparser.ConfigParser()
        impl_conf.read(self.impl_conf_path)

        self.enforce_bwlist_checksums = config.getboolean(
            'implementation', 'enforce_bwlist_checksums')

        self.implementations = _enum_prim_impls(
            self.operation,
            primitives,
            self.algopack_path
        )
        self._process_black_white_lists(config, impl_conf)

        self._enum_supercop_impls(config, impl_conf)
        self._enum_dependencies(config, impl_conf)

    def _process_black_white_lists(self, conf_parser, impl_conf_parser):
        impl_index = {}
        for i in self.implementations:
            impl_index[i.path] = i

        def parse_list(l, path_mangle):
            r = []
            for i in l:
                path,_,i = i.partition(" ")
                path = path_mangle(path.strip())
                hash,_,i = i.strip().partition(" ")
                hash = hash.strip()
                hash = hash if (hash != '0' and hash != '') else None
                comment = i.strip()
                r += (path, hash, comment),
            return r

        whitelist = []
        # Get config whitelist 
        config_whitelist = [i.strip() for i in conf_parser.get("implementation",
                                                       "whitelist").strip().split("\n")
                            if i]


        whitelist = parse_list(config_whitelist,
                               lambda path: os.path.join(self.operation.name,
                                                         os.path.normpath(path.strip())))

        if len(whitelist) > 0:
            impl_list = []

            for path, hash, comment in whitelist:
                try:
                    impl = impl_index[path]
                    if hash and hash != impl.hash:
                        errmsg = ("Checksum in whitelist entry for {} does not "+
                                  "match. Please update config.ini").format(path)
                        if self.enforce_bwlist_checksums:
                            raise ValueError(errmsg)
                        else:
                            _logger.warn(errmsg)
                    impl_list += impl,
                except KeyError:
                    pass

            self.implementations = impl_list
            self.whitelist = whitelist
            return

        blacklist = []
        blacklist_strings = []

        # Get global blacklists for all platforms
        try:
            blacklist_strings += ( i.strip() for i in impl_conf_parser.get(
                "ALL", "blacklist").strip().split("\n") if i)
        except configparser.NoOptionError:
            pass

        # Get global blacklists for current platform
        try:
            blacklist_strings += (i.strip() for i in impl_conf_parser.get(
                self.platform.name, "blacklist").strip().split("\n") if i)
        except configparser.NoSectionError:
            pass

        # Parse blacklist strings
        blacklist += parse_list(blacklist_strings,
                               lambda path: os.path.normpath(path))
        # Get config blacklist 
        config_blacklist = [i.strip() for i in conf_parser.get(
            "implementation", "blacklist").strip().split("\n") if i]
        blacklist += parse_list(config_blacklist,
                               lambda path: os.path.join(self.operation.name,
                                                         os.path.normpath(path)))

        for path, hash, comment in blacklist:
            try:
                impl = impl_index[path]

                if hash and hash != impl.hash:
                    errmsg = ("Checksum in blacklist entry for {} does not "
                              "match. Please update impl_conf.ini or "
                              "config.ini").format(path)
                    if self.enforce_bwlist_checksums:
                        raise ValueError(errmsg)
                    else:
                        _logger.warn(errmsg)
                try:
                    self.implementations.remove(impl)
                except ValueError:
                    pass

            except KeyError:
                pass


        # Filter out regex matches
        blacklist_regex = conf_parser.get("implementation", "blacklist_regex")
        if blacklist_regex:
            r = re.compile(blacklist_regex)
            # self.implementations = list(filter(lambda x: not bool(r.match(x.name)),
            #                                    self.implementations))
            self.implementations = [x for x in self.implementations if not r.match(x.name)]


        self.blacklist_regex=blacklist_regex
        self.blacklist = blacklist

    def _enum_dependencies(self, conf_parser, impl_conf_parser):
        # Index dependencies
        dependencies = {}
        for i in self.libsupercop_impls:
            dependencies[(i.primitive.operation.name, i.primitive.name)] = i

        dependents = {}
        # Index possible dependents
        for i in self.implementations:
            dependents[i.path] = i

        lsd = []
        try:
            lsd = (impl_conf_parser.
                   get('ALL', 'libsupercop_dependents').strip().split("\n"))
            # Ignore empty entries
            lsd = (i for i in lsd if i)

        except configparser.NoOptionError:
            pass

        for i in lsd:
            path,_,i = i.partition(" ")
            hash,_,i = i.strip().partition(" ")
            hash = hash.strip()
            deps = i.strip("[]").split(",")

            try:
                dependent = dependents[os.path.normpath(path.strip())]
            except KeyError:
                pass
            else:
                if dependent.hash != hash and hash != '0':
                    errmsg = ("Checksum for dependency entry for {} does not "
                              "match. Please update impl_conf.ini").format(path)
                    if self.enforce_bwlist_checksums:
                        raise ValueError(errmsg)
                    else:
                        _logger.warn(errmsg)

                for j in deps:
                    key = tuple(j.strip().split())
                    dependent.dependencies += dependencies[key],

    def _enum_supercop_impls(self, conf_parser, impl_conf_parser):

        libsupercop_impls = conf_parser.get("libsupercop", "implementations").strip().split("\n")
        operation_file    = conf_parser.get('paths','operations')

        operations = set()
        primitives = set()
        implementations = set()


        for i in libsupercop_impls:
            o, p, i = i.split(' ')
            operations.add(o)
            primitives.add((o,p))
            implementations.add((o,p,i))

        # convert operation names to operation objects
        operations = [_enum_operation(x, conf_parser.get("paths","operations")) for x in operations]

        all_impls = []

        for o in operations:
            all_impls += _enum_prim_impls(o, [p[1] for p in primitives if p[0] == o.name],
                                      self.algopack_path)

        self.libsupercop_impls = [i for i in all_impls if
                                  (i.primitive.operation.name, i.primitive.name, i.name) in implementations]




def _enum_prim_impls(operation, primitive_names, algopack_path):
    """Enumerations primitives and implementation, and sets lists of
    implementations in config

    Parameters:
        operation       Instance of Operation
        primitive_names List of primitive names
    """


    _logger.debug("Enumerating primitives and implementations")
    primitives = []
    implementations = []

    # Get operation path
    op_path = os.path.join(algopack_path, operation.name)

    if not os.path.isdir(op_path):
        raise ValueError(
            "Operation {} directory does not exist!".format(operation.name))

    # Get list of primitive directories
    primdirs = [d for d in os.listdir(op_path) if os.path.isdir(os.path.join(op_path, d))]

    # Get prim directories intersecting w/ primitive list
    for name in primitive_names :
        if name not in primdirs:
            raise ValueError("Primitive {} directory does not exist!".
                             format(name))

        path = os.path.join(op_path,name)
        try:
            checksumfile = os.path.join(path, "checksumsmall")
            checksumsmall = ""
            with open(checksumfile) as f:
                checksumsmall = f.readline().strip()

            checksumfile = os.path.join(path, "checksumbig")
            checksumbig= ""
            with open(checksumfile) as f:
                checksumbig = f.readline().strip()
        except FileNotFoundError:
            _logger.error(("Checksum(big|small) files not found for "
                           "{}/{}").format(operation.name,name))
            continue

        p = Primitive(
                operation=operation,
                name=name,
                path=os.path.relpath(path, algopack_path),
                checksumsmall=checksumsmall,
                checksumbig=checksumbig
            )

        primitives += p,

    for p in primitives:
        all_impls = {}
        p_fpath = os.path.join(algopack_path, p.path)

        # Find all directories w/ api.h
        walk =  os.walk(p_fpath)
        for dirpath, dirnames, filenames in walk:
            if "api.h" in filenames:
                path = os.path.relpath(dirpath, p_fpath)
                name = path.translate(str.maketrans("./-","___"))
                all_impls[name] = path

        for name in all_impls.keys():
            # Parse api.h and get macro values
            path = os.path.join(p_fpath,all_impls[name])
            checksum = dirchecksum(path)

            api_h = None
            with open(os.path.join(path, "api.h")) as f:
                api_h = f.read()

            macros = {}
            macro_names = operation.macro_names+['_VERSION']

            for m in macro_names:
                value = None
                match = re.search(m+r'\s+"(.*)"\s*$', api_h, re.MULTILINE)
                if(match):
                    value = match.group(1).strip('"')
                else:
                    match = re.search(m+r'\s+(.*)\s*$', api_h, re.MULTILINE)
                    try:
                        value = int(match.group(1))
                    except ValueError:
                        value = match.group(1)
                    except AttributeError:
                        value = None
                macros[m] = value

            # Don't have to add to p.implementations, as Implementation sets
            # Primitive as parent, which inserts into list
            implementations += Implementation(
                hash=checksum,
                name=name,
                path=os.path.relpath(path, algopack_path),
                primitive=p,
                macros=macros
            ),

    return implementations

def _enum_operation(name, filename):
    """Generate operation"""
    _logger.debug("Enumerating operations")

    with open(filename) as f:
        for l in f:
            match = re.match(r'(\w+) ([:_\w]+) (.*)$', l)
            if match.group(1) == name:
                return Operation(name=name, operation_str=l.strip())




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
        config_path,
        config.get('paths','operations'),
        config.get('paths','impl_conf'),
    )

    dir_paths = (
        config.get('paths','platforms'),
        config.get('paths','algopacks'),
        config.get('paths','embedded'),
    )

    dir_hashes = list(map(lambda x: dirchecksum(x), dir_paths))
    file_hashes = list(map(lambda x: xbx.util.sha256_file(x), file_paths))

    h = hashlib.sha256()
    for f in (dir_hashes+file_hashes):
        h.update(f.encode())

    h.update(config_hash.encode())

    config_hash_cache[config_hash] = h.hexdigest()

    return config_hash_cache[config_hash]

