import logging
import os
import re
import string
import configparser
import subprocess
import sys
import threading
import datetime

import xbx.buildfiles as buildfiles

EXE_NAME="xbdprog.bin"
HEX_NAME="xbdprog.hex"

class Stats:
    def __init__(self, 
            operation,
            primitive,
            implementation,
            compiler_idx,
            cc,
            cxx,
            hex_path,
            exe_path,
            workpath,
            timestamp,
            text, data, bss):

        self.operation      = operation
        self.primitive      = primitive
        self.implementation = implementation
        self.compiler_idx   = compiler_idx
        self.cc             = cc
        self.cxx            = cxx
        self.hex_path       = hex_path
        self.exe_path       = exe_path
        self.workpath       = workpath

        self.text = text
        self.data = data
        self.bss  = bss 

class Build:
    """Sets up a build
    
    Parameters:
        config          xbx.Config object
        index           compiler index
        implementation  xbx.Config.Implementation instance
        warn_comp_err   If false, Compiler errs have DEBUG loglevel instead of
                        WARN
        parallel        If true, issues -j flag to make 
    """
    def __init__(self, config, compiler_idx, implementation,# {{{
            warn_comp_err=False, parallel=False):
        self.config = config
        self.cc = config.platform.compilers[compiler_idx]['CC']
        self.cxx = config.platform.compilers[compiler_idx]['CXX']
        self.compiler_idx = compiler_idx
        self.implementation = implementation
        self.workpath = os.path.join(
                config.work_path,
                config.platform.name,
                config.operation.name,
                self.implementation.primitive.name,
                self.implementation.name,
                str(compiler_idx))
        self.exe_path = os.path.join(self.workpath, EXE_NAME)
        self.hex_path = os.path.join(self.workpath, HEX_NAME)
        self.warn_comp_err = warn_comp_err
        self.parallel = False

        operation = self.implementation.primitive.operation
        primitive = self.implementation.primitive

        self.buildid = "{}/{}/{}/{}".format(
            operation.name,
            primitive.name,
            implementation.name,
            self.compiler_idx)

        # Set build environment variables
        tmpl_path = self.config.platform.tmpl_path
        self.env = {'CC': self.cc,
            'templatePlatformDir': tmpl_path if tmpl_path else '',
            'CXX': self.cxx,
            'HAL_PATH': os.path.join(self.config.platform.path,'hal'),
            'HAL_T_PATH': os.path.join(tmpl_path,'hal') if tmpl_path else '',
            'XBD_PATH': os.path.join(self.config.embedded_path,'xbd'),
            'IMPL_PATH': implementation.path,
            'OP': operation.name,
            'POSTLINK': os.path.join(self.config.platform.path, 'postlink'),
            'HAL_OBJS': os.path.join(
                config.work_path,
                config.platform.name,
                'HAL',
                str(compiler_idx))}

        self.log_attr = {'buildid': self.buildid}
        self.gen_files()
        self.stats = None
        self.timestamp = None

    # }}}

    def compile(self):# {{{
        logger = logging.getLogger(__name__+".Build")
        #logger.info("Building {} for platform {}".format(
        #    self.buildid,
        #    self.config.platform.name), extra=self.log_attr)

        self.make("all")

        if os.path.isfile(self.exe_path):

            size = os.path.join(self.config.platform.path, 'size')
            total_env = self.env.copy()
            total_env.update(os.environ)
            stdout = subprocess.check_output((size, self.exe_path),
                    env=total_env)
            sizeout = stdout.decode().splitlines()[1]
            match = re.match(r'^\s*(\w+)\s+(\w+)\s+(\w+)', sizeout)
            self.timestamp = datetime.datetime.now()
            self.stats = Stats(
                    operation      = self.implementation.primitive.operation,
                    primitive      = self.implementation.primitive,
                    implementation = self.implementation,
                    compiler_idx   = self.compiler_idx,
                    cc             = self.cc,
                    cxx            = self.cxx,
                    hex_path       = self.hex_path,
                    exe_path       = self.exe_path,
                    workpath       = self.workpath,
                    timestamp       = self.timestamp,
                    text           = int(match.group(1)),
                    data           = int(match.group(2)),
                    bss            = int(match.group(3)))
            logger.info("SUCCESS building {}".format(self.buildid), extra=self.log_attr)
        else:
            logger.info("FAILURE building {}".format(self.buildid), extra=self.log_attr)

    # }}}

    def clean(self):# {{{
        logger = logging.getLogger(__name__+".Build")
        logger.debug("Cleaning "+self.buildid+"...", extra=self.log_attr)
        self.make("clean")
    # }}}

    def make(self, target):# {{{

        logger = logging.getLogger(__name__+".Build")
        err_logger = logger.warn if self.warn_comp_err else logger.debug

        _make(self.workpath, logger.debug, err_logger, target, self.parallel, extra=self.log_attr)
    # }}}

    def gen_files(self):# {{{
        operation = self.implementation.primitive.operation
        primitive = self.implementation.primitive

        # Generate files 
        try:
            os.makedirs(self.workpath)
        except OSError:
            pass

        _gen_envfile(self.workpath, self.env)

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
        #self.logger.debug("Generating Makefile...", extra=self.log_attr)
        with open(filename, 'w') as f:
            f.write(buildfiles.MAKEFILE)
    # }}}
        
    def _gen_crypto_o_h(self, filename, primitive):# {{{
        #self.logger.debug("Generating "+filename+"...", extra=self.log_attr)
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
        #self.logger.debug("Generating "+filename+"...", extra=self.log_attr)
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

def build_hal(config, index):
    """Builds HAL for given compiler index"""
    logger = logging.getLogger(__name__)

    logger.info("Building HAL for platform {}, compiler {}".format(
            config.platform.name,
            str(index)))


    workpath = os.path.join(
            config.work_path,
            config.platform.name,
            'HAL',
            str(index))

    tmpl_path = config.platform.tmpl_path

    try:
        os.makedirs(workpath)
    except OSError:
        pass

    makefile = os.path.join(workpath, 'Makefile')

    if not os.path.isfile(makefile):
        with open(makefile, 'w') as f:
            f.write(buildfiles.HAL_MAKEFILE)


    env = ({'CC': config.platform.compilers[index]['CC'],
        'templatePlatformDir': tmpl_path if tmpl_path else '',
        'XBD_PATH': os.path.join(config.embedded_path,'xbd'),
        'HAL_PATH': os.path.join(config.platform.path,'hal'),
        'HAL_T_PATH': os.path.join(tmpl_path,'hal') if tmpl_path else ''})

    #_gen_envfile(workpath, env)

    _make(workpath, logger.debug, logger.debug, parallel = True)



def _make(path, log_fn, err_log_fn, target="all", parallel=False, extra=[]):
    """Runs subprocess logging output

    Parameters:
    cmd         Command given to subprocess.Popen()
    path        Path to chdir before execution
    env         Environment for command
    log_fn      Logging function for stdout
    err_logfn   Logging function for stderr
    extra       Extra to send to logger
    """

    cmd = ["make"]
    if parallel:
        cmd += "-j",
    cmd += target,

    old_pwd = os.getcwd()
    os.chdir(path)
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    os.chdir(old_pwd)

    def read_thread(fd, log_fn):
        for l in iter(fd.readline, b''):
            log_fn(l.decode().strip("\n"), extra=extra)

    stdout_t = threading.Thread(target=read_thread, args=(process.stdout, log_fn))
    stderr_t = threading.Thread(target=read_thread, args=(process.stderr, err_log_fn))

    stdout_t.daemon = True
    stderr_t.daemon = True
    stdout_t.start()
    stderr_t.start()
    process.wait()

def _gen_envfile(path, env):
    """Takes dirpath of env.make and dictionary of environment variables"""
    envfile = os.path.join(path, 'env.make')

    if os.path.isfile(envfile):
        old_env = _parse_envfile(envfile)
        if set(old_env.items()) == set(env.items()):
            return True

    with open(envfile, 'w') as f:
        for key, value in env.items():
            f.write("{}={}\n".format(key,value))
    return False
    
def _parse_envfile(path):
    env = {}
    with open(path) as f:
        for l in f:
            name, _, value = l.partition("=")
            env[name.strip()]=value.strip("\n")
    return env
    


