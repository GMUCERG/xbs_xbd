import logging
import os
import re
import string
import subprocess
import sys
import threading
import datetime
import hashlib
import multiprocessing as mp

import xbx.buildfiles as buildfiles
import xbx.util
import xbx.data as data
from xbx.dirchecksum import dirchecksum

EXE_NAME="xbdprog.bin"
HEX_NAME="xbdprog.hex"

class Build:# {{{
    """Sets up a build
    
    Parameters:
        config          xbx.Config object
        index           compiler index
        implementation  xbx.Config.Implementation instance
        warn_comp_err   If false, Compiler errs have DEBUG loglevel instead of
                        WARN
        parallel        If true, issues -j flag to make 

    """
    def __init__(
            self, config, compiler_idx, implementation, warn_comp_err=False,
            parallel=False):
        #self.config = config
        self.cc = config.platform.compilers[compiler_idx].cc
        self.cxx = config.platform.compilers[compiler_idx].cxx
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

        self.primitive = self.implementation.primitive
        self.operation = self.primitive.operation
        self.platform = config.platform

        self.buildid = "{}/{}/{}/{}".format(
            self.operation.name,
            self.primitive.name,
            self.implementation.name,
            self.compiler_idx)

        # Set build environment variables
        tmpl_path = self.platform.tmpl_path
        self.env = {
                'templatePlatformDir': tmpl_path if tmpl_path else '',
                'HAL_PATH':            os.path.join(self.platform.path,'hal'),
                'HAL_T_PATH':          os.path.join(tmpl_path,'hal') if tmpl_path else '',
                'XBD_PATH':            os.path.join(config.embedded_path,'xbd'),
                'IMPL_PATH':           implementation.path,
                'POSTLINK':            os.path.join(self.platform.path, 'postlink'),
                'HAL_OBJS':            os.path.join(
                    config.work_path,
                    config.platform.name,
                    'HAL',
                    str(compiler_idx))}

        # Make paths absolute, except for compilers
        for k, v in self.env.items():
            if v:
                self.env[k] = os.path.abspath(v)

        # Add compilers and non path env variables
        self.env.update({
            'OP': self.operation.name,
            'CC': self.cc, 'CXX': self.cxx})
                
        self.log_attr = {'buildid': self.buildid}
        self.stats = None
        self.timestamp = None
        self.checksum = None

        self.text = -1
        self.data = -1
        self.bss  = -1

        self.impl_checksum = None 
        self.hex_checksum = None
    

    def compile(self):
        self._gen_files()
        logger = logging.getLogger(__name__+".Build")
        self.impl_checksum = dirchecksum(self.workpath)
        #logger.info("Building {} for platform {}".format(
        #    self.buildid,
        #    self.config.platform.name), extra=self.log_attr)

        self.make("all")

        self.timestamp = datetime.datetime.now()
        if os.path.isfile(self.hex_path):
            size = os.path.join(self.platform.path, 'size')
            total_env = self.env.copy()
            total_env.update(os.environ)
            stdout = subprocess.check_output((size, self.exe_path),
                    env=total_env)
            sizeout = stdout.decode().splitlines()[1]
            match = re.match(r'^\s*(\w+)\s+(\w+)\s+(\w+)', sizeout)

            self.text = match.group(1)
            self.data = match.group(2)
            self.bss  = match.group(3)

            self.hex_checksum = xbx.util.sha256_file(self.hex_path)


            logger.info("SUCCESS building {}".format(self.buildid), extra=self.log_attr)
        else:
            logger.info("FAILURE building {}".format(self.buildid), extra=self.log_attr)

    

    def clean(self):
        logger = logging.getLogger(__name__+".Build")
        logger.debug("Cleaning "+self.buildid+"...", extra=self.log_attr)
        self.make("clean")
    

    def make(self, target):

        logger = logging.getLogger(__name__+".Build")
        err_logger = logger.warn if self.warn_comp_err else logger.debug

        _make(self.workpath, logger.debug, err_logger, target, self.parallel, extra=self.log_attr)
    

    def _gen_files(self):
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
    
        
    def _genmake(self, filename):
        #self.logger.debug("Generating Makefile...", extra=self.log_attr)
        with open(filename, 'w') as f:
            f.write(buildfiles.MAKEFILE)
    
        
    def _gen_crypto_o_h(self, filename, primitive):
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
    

    def _gen_crypto_op_h(self, filename, implementation):
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

class BuildSession:# {{{
    """Manages builds for all instances specified in xbx config"""
    CPU_COUNT = mp.cpu_count()

    def __init__(self, config, database):
        logger = logging.getLogger(__name__+".Build")
        self.config = config
        self.database = database
        self.builds = []

        try:
            self.xbx_version = subprocess.check_output(
                    ["git", "rev-parse", "HEAD"]
                    ).decode().strip()
        except subprocess.CalledProcessError:
            logger.warn("Could not get git revision of xbx")

        self.session_id = self.database.save_buildsession(self)



    def buildall(self):
        """Builds all targets specified in xbx self.config, and saves stats into cursor"""

        logger = logging.getLogger(__name__)
        num_compilers = len(self.config.platform.compilers)

        for i in range(num_compilers):
            compiler = self.config.platform.compilers[i]
            logger.info("compiler[{}] = {}".format(i, str((compiler.cc, compiler.cxx))))
            if self.config.one_compiler:
                break

        for i in range(num_compilers):
            build_hal(self.config, i)
            if self.config.one_compiler:
                break

        for p in self.config.primitives:
            for j in p.impls:
                for i in range(num_compilers):
                    build = Build(self.config, i, j)
                    self.builds += build,

                    if self.config.one_compiler:
                        break
        
        if self.config.parallel_build:
            q_out = mp.Queue()
            q_in = mp.Queue()

            def worker(q_in, q_out, n):
                logger.info("Worker "+str(n)+" started")
                for build in iter(q_in.get, None):
                    build.compile()
                    q_out.put(build)
                logger.info("Worker "+str(n)+" finished")

            processes = [mp.Process(target=worker, args=(q_out,q_in, i)) 
                    for i in range(BuildSession.CPU_COUNT+1)]
            for p in processes:
                p.start()

            for b in self.builds:
                q_out.put(b)

            # Clear out old build list, reobtain from queue with updated data
            num_builds = len(self.builds)
            self.builds = []
            for _ in range(num_builds):
                b = q_in.get()
                self.database.save_build(b, self)
                self.builds += b,

            # Terminate processes
            for p in processes:
                q_out.put(None)
        else:
            for b in self.builds:
                b.compile()
                self.database.save_build(b, self)

        self.database.commit()

# }}}
    
# Support fxns# {{{
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


    env = ({
        'templatePlatformDir': tmpl_path if tmpl_path else '',
        'XBD_PATH': os.path.join(config.embedded_path,'xbd'),
        'HAL_PATH': os.path.join(config.platform.path,'hal'),
        'HAL_T_PATH': os.path.join(tmpl_path,'hal') if tmpl_path else ''})

    for k, v in env.items():
        if v:
            env[k] = os.path.abspath(v)

    env.update({'CC': config.platform.compilers[index].cc})
    
    _gen_envfile(workpath, env)

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
# }}}
