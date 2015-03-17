import logging
import os
import re
import shutil
import string
import configparser
import subprocess
import sys
import threading

import xbx.buildfiles as buildfiles

EXE_NAME="xbdprog.bin"
HEX_NAME="xbdprog.hex"

class Stats:
    def __init__(self, text, data, bss):
        text = text
        data = data
        bss = bss 

class Build:
    """Sets up a build
    
    Parameters:
        config          xbx.Config object
        index      compiler index
        implementation  xbx.Config.Implementation instance
        warn_comp_err   If false, Compiler errs have DEBUG loglevel instead of
                        WARN
    """

    def __init__(self, config, index, implementation,# {{{
            warn_comp_err=False):
        self.config = config
        self.cc = config.platform.compilers[index]['cc']
        self.cxx = config.platform.compilers[index]['cxx']
        self.index = index
        self.implementation = implementation
        self.workpath = os.path.join(
                config.work_path,
                config.platform.name,
                self.implementation.primitive.name,
                self.implementation.name,
                str(index))
        self.exe_path = os.path.join(self.workpath, EXE_NAME)
        self.hex_path = os.path.join(self.workpath, HEX_NAME)
        self.warn_comp_err = warn_comp_err

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
            'POSTLINK': os.path.join(self.config.platform.path, 'postlink'),
            'HAL_OBJS': os.path.join(
                config.work_path,
                config.platform.name,
                'HAL',
                str(index))})


        #self.logger = logger
        self.log_attr = {'buildid': self.buildid}

        self.gen_files()

        self.compiled = False

    # }}}

    def compile(self):# {{{
        #self.logger.info("Building {} on platform {} with:\n\tCC={}\n\tCXX={}".format(
        #    self.buildid,
        #    self.config.platform.name,
        #    self.cc,
        #    self.cxx), extra=self.log_attr)
        self.make("all")

        if os.path.isfile(self.exe_path):
            self.compiled = True

            size = os.path.join(self.config.platform.path, 'size')
            stdout = ""
            try:
                stdout = subprocess.check_output((size, self.exe_path), env=self.env)
                sizeout = stdout.decode().splitlines()[1]
                match = re.match(r'^\s*(\w+)\s+(\w+)\s+(\w+)', sizeout)
                self.stats = Stats(
                    int(match.group(1)), 
                    int(match.group(2)), 
                    int(match.group(3)))
            except subprocess.CalledProcessError as cpe:
                #self.logger.error(cpe.cmd, extra=self.log_attr)
                #self.logger.error(cpe.returncode, extra=self.log_attr)
                #self.logger.error(cpe.output, extra=self.log_attr)
                shutil.copytree(self.workpath, "/tmp/err")
                #stdout = subprocess.check_output((size, self.exe_path), env=self.env)
                #print(stdout)
                os._exit(1)


    # }}}

    def clean(self):# {{{
        #self.logger.debug("Cleaning "+self.buildid+"...", extra=self.log_attr)
        self.make("clean")
    # }}}

    def purge(self):
        pass

    def make(self, target):# {{{
        makecmd = ['make', '-j', target]

        # Run make
        old_pwd = os.getcwd()
        os.chdir(self.workpath)
        process = subprocess.Popen(makecmd, env=self.env, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        os.chdir(old_pwd)

        def read_thread(fd, log_fn):
            for l in iter(fd.readline, b''):
                print(l.decode().strip("\n"))
                #log_fn(l.decode().strip("\n"), extra=self.log_attr)

        #stdout_t = threading.Thread(target=read_thread, args=(process.stdout, self.logger.debug))
        #stderr_t = threading.Thread(target=read_thread, args=(process.stderr, 
        #        self.logger.warn if self.warn_comp_err else self.logger.debug))
        stdout_t = threading.Thread(target=read_thread, args=(process.stdout,None))
        stderr_t = threading.Thread(target=read_thread, args=(process.stderr, None))

        stdout_t.daemon = True
        stderr_t.daemon = True
        stdout_t.start()
        stderr_t.start()

        # Wait for make to finish
        process.wait()
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
    logger = logging.getLogger(__name__)

    logger.info("Building HAL for platform {}, compindex {}".format(
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

    if True: #not os.path.isfile(makefile):
        with open(makefile, 'w') as f:
            f.write(buildfiles.HAL_MAKEFILE)

    env = os.environ.copy()

    env.update({'CC': config.platform.compilers[index]['cc'],
        'templatePlatformDir': tmpl_path if tmpl_path else '',
        'XBD_PATH': os.path.join(config.embedded_path,'xbd'),
        'HAL_PATH': os.path.join(config.platform.path,'hal'),
        'HAL_T_PATH': os.path.join(tmpl_path,'hal') if tmpl_path else '',
        'HAL_OBJS': workpath})

    makecmd = ['make', '-j']

    old_pwd = os.getcwd()
    os.chdir(workpath)
    #process = subprocess.Popen(makecmd, env=env)
    process = subprocess.Popen(makecmd, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    os.chdir(old_pwd)

    def read_thread(fd, log_fn):
        for l in iter(fd.readline, b''):
            log_fn(l.decode().strip("\n"))

    stdout_t = threading.Thread(target=read_thread, args=(process.stdout, logger.debug))
    stderr_t = threading.Thread(target=read_thread, args=(process.stderr, logger.debug))

    stdout_t.daemon = True
    stderr_t.daemon = True
    stdout_t.start()
    stderr_t.start()
    process.wait()
