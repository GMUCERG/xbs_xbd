#! /usr/bin/python3
import logging
import os
import re
import shutil
import string
import configparser
import subprocess
import sys
import distutils.util


_logger=logging.getLogger(__name__)

class Platform:
    def __init__(self, name, path, tmpl_path, clock_hz, pagesize, compilers):
        self.name = name
        self.path = path
        self.tmpl_path = tmpl_path
        self.clock_hz = clock_hz
        self.pagesize = pagesize
        self.compilers = compilers

    def __lt__(self, other):
        return self.name < other.name

class Operation:
    def __init__(self, name, macros, prototypes):
        self.name = name
        self.macros = macros
        self.prototypes = prototypes

    def __lt__(self, other):
        return self.name < other.name

class Primitive:
    def __init__(self, name, operation, path, checksumsmall):
        self.name = name
        self.operation = operation
        self.path = path
        self.checksumsmall = checksumsmall
        self.impls = []


    def __lt__(self, other):
        return self.name < other.name

class Implementation:
    def __init__(self, name, primitive, path):
        self.name = name
        self.primitive = primitive
        self.path = path

    def __lt__(self, other):
        return self.name < other.name


class Config:# {{{
    """Configuration for running benchmarks on a single operation on a single platform"""

    def __init__(self, filename):# {{{
        config = configparser.ConfigParser()
        config.read(filename)

        ## Basic path configuration
        self.platforms_path = os.path.abspath(config.get('paths','platforms'))
        self.algopack_path  = os.path.abspath(config.get('paths','algopacks'))
        self.embedded_path  = os.path.abspath(config.get('paths','embedded'))
        self.work_path      = os.path.abspath(config.get('paths','work'))

        self.one_compiler = bool(distutils.util.strtobool(config.get('run', 'one_compiler')))

        self.loglevel = Config.__loglevel(config.get('run','loglevel'))


        # XBH address
        self.xbh_addr = config.get('xbh', 'address')

        # Platform
        self.platform = Config.__enum_platform(
                config.get('hardware','platform'),
                self.platforms_path)

        # Operation
        name = config.get('algorithm','operation')
        filename = config.get('paths','operations')
        self.operation = Config.__enum_operation(name, filename)


        # TODO Read platform blacklists
        # Update platform blacklist sha256sums

        blacklist = []
        whitelist = []

        if config.has_option('implementation', 'blacklist') and config.get('implementation','blacklist'):
            blacklist = config.get('implementation','blacklist').split("\n")

        if config.has_option('implementation', 'whitelist') and config.get('implementation','whitelist'):
            whitelist = config.get('implementation','whitelist').split("\n")

        primitives = config.get('algorithm','primitives').split("\n")
        self.primitives = Config.__enum_prim_impls(
                self.operation, 
                primitives, 
                blacklist,
                whitelist, 
                self.algopack_path)


    # }}}
    @staticmethod
    def __loglevel(name):# {{{
        if name == "DEBUG":
            return logging.DEBUG
        elif name == "INFO":
            return logging.INFO
        elif name == "WARNING":
            return logging.WARNING
        elif name == "ERROR":
            return logging.ERROR
        elif name == "CRITICAL":
            return logging.CRITICAL

    @staticmethod
    def __enum_platform(name, platforms_path):# {{{
        """Enumerate platform settings, given path to platform directory"""

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

        return Platform(name, path, tmpl_path, clock_hz, pagesize,
                compilers)

    # }}}



    @staticmethod
    def __enum_compilers(platform_path, tmpl_path):# {{{
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

        for i in range(0, len(cc_list)):
            compilers += ({'cc': cc_list[i], 'cxx': cxx_list[i]},)

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
            p = Primitive(name, operation, path, checksumsmall)

            primitives += p, 

        primitives = sorted(primitives)


        # Get whitelist  ^blacklist
        for p in primitives:
            alldirs = [] 

            # Find all directories w/ api.h
            walk =  os.walk(p.path)
            for i in walk:
                if "api.h" in i[2]:
                    alldirs += os.path.relpath(i[0], p.path),

            keptdirs = set(alldirs)

            for i in alldirs:
                for j in blacklist:
                    if re.match(j,i):
                        keptdirs.remove(i)

            if whitelist:
                keptdirs &= set(whitelist);

            for name in keptdirs:
                path = os.path.join(p.path,name)
                impl = Implementation(name,p,path)
                p.impls += (impl,)

            p.impls = sorted(p.impls)

        return primitives

    # }}}

    @staticmethod
    def __enum_operation(name, filename):# {{{
        """Generate operation"""

        macros = []
        prototypes = []
        
        with open(filename) as f:
            for l in f:
                match = re.match(r'crypto_(\w+) ([:_\w]+) (.*)$', l)
                if match.group(1) == name:
                    macros  += match.group(2).split(':')
                    prototypes += match.group(3).split(':')
                    break
        # Add empty string to macro list
        macros += '',
        return Operation(name, macros, prototypes)
    # }}}

# }}}


