#! /usr/bin/python3
import logging
import os
import re
import shutil
import string
import configparser
import subprocess
import sys
import threading

import buildfiles

_logger=logging.getLogger(__name__)

EXE_NAME="xbdprog.bin"
HEX_NAME="xbdprog.hex"

class Config:# {{{
    """Configuration for running benchmarks on a single operation on a single
    platform"""

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


    xbh_addr = ""
    

    def __init__(self, filename):# {{{
        config = configparser.ConfigParser()
        config.read(filename)

        ## Basic path configuration
        self.platforms_path = os.path.abspath(config.get('paths','platforms'))
        self.algopack_path  = os.path.abspath(config.get('paths','algopacks'))
        self.embedded_path  = os.path.abspath(config.get('paths','embedded'))
        self.work_path      = os.path.abspath(config.get('paths','work'))

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

        return Config.Platform(name, path, tmpl_path, clock_hz, pagesize,
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
            process = subprocess.Popen([c_compilers], 
                env=env, stdout=subprocess.PIPE)
            stdout, stderr = process.communicate()
            cc_list += stdout.splitlines()
        if os.path.isfile(cxx_compilers):
            process = subprocess.Popen([cxx_compilers], 
                env=env, stdout=subprocess.PIPE)
            stdout, stderr = process.communicate()
            cxx_list += stdout.splitlines()

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
            operation       Instance of Config.Operation
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
            p = Config.Primitive(name, operation, path, checksumsmall)

            primitives += (p, )

        primitives = sorted(primitives)


        # Get whitelist  ^blacklist
        for p in primitives:
            alldirs = [d for d in os.listdir(p.path) if
                    os.path.isdir(os.path.join(p.path, d))]

            keptdirs = set(alldirs)

            for i in alldirs:
                for j in blacklist:
                    if re.match(j,i):
                        keptdirs.remove(i)

            if whitelist:
                keptdirs &= set(whitelist);

            for name in keptdirs:
                path = os.path.join(p.path,name)
                impl = Config.Implementation(name,p,path)
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
        macros += ('',)
        return Config.Operation(name, macros, prototypes)
    # }}}

# }}}

class Build:# {{{
    """Sets up a build
    
    Parameters:
        config          xbx.Config object
        cc              c compiler and flags
        cxx             c++ compiler and flags
        comp_index      compiler index
        implmentation   xbx.Config.Implementation instance
        verbose         True if verbosity desired"""

    class Stats:
        def __init__(self, text, data, bss):
            text = text
            data = data
            bss = bss 

    def __init__(self, config, cc, cxx, index, implementation,# {{{
            verbose=False, log_fd=sys.stdout):
        self.config = config
        self.cc = cc
        self.cxx = cxx
        self.index = index
        self.implementation = implementation
        self.verbose = verbose
        self.workpath = os.path.join(
                config.work_path,
                config.platform.name,
                self.implementation.primitive.name,
                self.implementation.name,
                str(index))
        self.exe_path = os.path.join(self.workpath, EXE_NAME)
        self.hex_path = os.path.join(self.workpath, HEX_NAME)

        operation = self.implementation.primitive.operation
        primitive = self.implementation.primitive
        self.buildid = "{}/{}/{}:{}".format(
            operation.name,
            primitive.name,
            implementation.name,
            self.index)

        # Set build environment variables
        tmpl_path = self.config.platform.tmpl_path
        self.env = os.environ.copy()
        self.env.update({'CC': self.cc,
            'templatePlatformDir': tmpl_path if tmpl_path else '',
            'CXX': self.cxx,
            'HAL_PATH': os.path.join(self.config.platform.path,'hal'),
            'HAL_T_PATH': os.path.join(tmpl_path,'hal') if tmpl_path else '',
            'XBD_PATH': os.path.join(self.config.embedded_path,'xbd'),
            'IMPL_PATH': implementation.path,
            'OP': operation.name,
            'POSTLINK': os.path.join(self.config.platform.path, 'postlink')})

        self.logger = logging.getLogger(__name__+".Build")
        self.log_attr = {'buildid': self.buildid}

        self.gen_files()

    # }}}

    def compile(self):# {{{
        self.logger.info("Building {} on platform {} with:\n\tCC={}\n\tCXX={}".format(
            self.buildid,
            self.config.platform.name,
            self.cc,
            self.cxx), extra=self.log_attr)
        self.make("all")

        size = os.path.join(self.config.platform.path, 'size')
        process = subprocess.Popen((size, self.exe_path), env=self.env, stdout=subprocess.PIPE)
        stdout, stdin = process.communicate()
        sizeout = stdout.decode().splitlines()[1]
        match = re.match(r'^\s*(\w+)\s+(\w+)\s+(\w+)', sizeout)
        self.stats = Build.Stats(match.group(1), match.group(2), match.group(3))


    # }}}

    def clean(self):# {{{
        self.logger.debug("Cleaning "+self.buildid+"...", extra=self.log_attr)
        self.make("clean")
    # }}}

    def purge(self):
        pass

    def make(self, target):# {{{
        makecmd = ['make','-j', target]

        # Run make
        old_pwd = os.getcwd()
        os.chdir(self.workpath)
        process = subprocess.Popen(makecmd, env=self.env, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        os.chdir(old_pwd)

        stop = False
        def read_thread(fd, log_fn):
            while not stop:
                line = fd.readline().decode()
                if line:
                    log_fn(line.strip("\n"), extra=self.log_attr)

        threading.Thread(target=read_thread, args=(process.stdout, self.logger.debug)).start()
        threading.Thread(target=read_thread, args=(process.stderr, self.logger.warn)).start()

        # Wait for make to finish
        process.wait()
        stop = True
    # }}}

    def gen_files(self):# {{{
        operation = self.implementation.primitive.operation
        primitive = self.implementation.primitive

        # Generate files 
        try:
            os.makedirs(self.workpath)
        except OSError:
            pass

        makefile = os.path.join(self.workpath, 'Makefile')
        crypto_o_h = os.path.join(self.workpath, "crypto_"+operation.name+".h")
        crypto_op_h = os.path.join(self.workpath, 
                "crypto_"+operation.name + "_" + primitive.name+".h")
        if not os.path.isfile(makefile):
            self._genmake(makefile)
        if not os.path.isfile(crypto_o_h):
            self._gen_crypto_o_h(crypto_o_h, primitive)
        if not os.path.isfile(crypto_op_h):
            self._gen_crypto_op_h(crypto_op_h, self.implementation)
    # }}}
                
        
    def _genmake(self, filename):# {{{
        self.logger.debug("Generating Makefile...", extra=self.log_attr)
        with open(filename, 'w') as f:
            f.write(buildfiles.MAKEFILE)
    # }}}
        
    def _gen_crypto_o_h(self, filename, primitive):# {{{
        self.logger.debug("Generating "+filename+"...", extra=self.log_attr)
        macro_expand = []
        o = 'crypto_'+primitive.operation.name 
        op = o + '_'+primitive.name

        for m in primitive.operation.macros:
            macro_expand+=("#define {}{} {}{}".format(
                o, m, op, m),)

        subst_dict = {
                'op': op,
                'o': o,
                'p': primitive.name,
                'op_macros': '\n'.join(macro_expand)
                }

        with open(filename, 'w') as f:
            f.write(string.Template(buildfiles.CRYPTO_O_H).substitute(subst_dict))
    # }}}

    def _gen_crypto_op_h(self, filename, implementation):# {{{
        self.logger.debug("Generating "+filename+"...", extra=self.log_attr)
        operation = implementation.primitive.operation
        primitive = implementation.primitive

        macro_expand = []
        prototype_expand = []
        o = 'crypto_'+primitive.operation.name 
        op = o + '_'+primitive.name
        opi = op + '_' + implementation.name
        api_h = ""

        for m in operation.macros:
            macro_expand+=("#define {}{} {}{}".format( op, m, opi, m),)

        for pr in operation.prototypes:
            prototype_expand+=("extern int {}{};".format(opi, pr),)

        with open(os.path.join(implementation.path, "api.h")) as f:
            api_h = f.read()
            api_h = api_h.replace("CRYPTO_", opi+'_')

        subst_dict = {
                'i': implementation.name,
                'api_h': api_h,
                'opi': opi,
                'op': op,
                'o': o,
                'p': primitive.name,
                'op_macros': '\n'.join(macro_expand),
                'opi_prototypes': '\n'.join(prototype_expand)
                }

        with open(filename, 'w') as f:
            f.write(string.Template(buildfiles.CRYPTO_OP_H).substitute(subst_dict))
    # }}}
# }}}


