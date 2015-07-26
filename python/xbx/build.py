import atexit
from datetime import datetime
import hashlib
import logging
import multiprocessing as mp
import os
import re
import string
import subprocess
import sys
import threading


from sqlalchemy.schema import ForeignKeyConstraint, PrimaryKeyConstraint, UniqueConstraint
from sqlalchemy import Column, ForeignKey, Integer, String, Text, Boolean, DateTime
from sqlalchemy.orm import relationship


import xbx.util
import xbx.session as xbxs
from xbx.dirchecksum import dirchecksum
from xbx.database import Base
import xbx.database as xbxdb


EXE_NAME="xbdprog.bin"
HEX_NAME="xbdprog.hex"

_logger = logging.getLogger(__name__)
_buildjob_logger = logging.getLogger(__name__+".BuildJob")

class BuildJob:
    """Separate object for actually building

    Build does not serialize well, thus obtain an instance of this with
    Build.get_buildjob() and run this instead

    """

    def __init__(self, work_path, parallel_make, log_attr, buildid,
                 platform_name, hex_path):
        self.timestamp = None
        self.work_path = work_path
        self.parallel_make = parallel_make
        self.log_attr = log_attr
        self.buildid = buildid
        self.platform_name = platform_name
        self.hex_path = hex_path
        self.log = None

    def __call__(self):
        _buildjob_logger.debug("Building {} for platform {}".format(
            self.buildid,
            self.platform_name), extra=self.log_attr)

        self.log = _make(self.work_path, _buildjob_logger.debug, "all",
              self.parallel_make, extra=self.log_attr)

        if os.path.isfile(self.hex_path):
            self.timestamp = datetime.now()
            _buildjob_logger.info("SUCCESS building {}".format(self.buildid), extra=self.log_attr)
        else:
            _buildjob_logger.info("FAILURE building {}".format(self.buildid), extra=self.log_attr)

class Build(Base):
    """Sets up a build

    Parameters:
        config          xbx.Config object
        index           compiler index
        implementation  xbx.Config.Implementation instance
        parallel_make   If true, issues -j flag to make

    """
    __tablename__ = "build"

    id                   = Column(Integer, nullable=False)
    build_session_id     = Column(Integer, nullable=False)

    platform_hash        = Column(String, nullable=False)
    compiler_idx         = Column(Integer, nullable=False)

    operation_name       = Column(String, nullable=False)
    primitive_name       = Column(String, nullable=False)
    implementation_hash  = Column(String, nullable=False)

    work_path            = Column(String)
    exe_path             = Column(String)
    hex_path             = Column(String)

    parallel_make        = Column(Boolean)

    text                 = Column(Integer)
    data                 = Column(Integer)
    bss                  = Column(Integer)

    timestamp            = Column(DateTime)
    hex_checksum         = Column(String)

    rebuilt              = Column(Boolean)
    build_ok             = Column(Boolean)

    log                  = Column(Text)

    platform             = relationship("Platform", uselist=False,
                                        foreign_keys=[platform_hash])

    compiler             = relationship("Compiler", uselist=False, viewonly=True)
    operation            = relationship("Operation", uselist=False,
                                        viewonly=True)
    primitive            = relationship("Primitive", uselist=False)
    implementation       = relationship("Implementation", uselist=False)


    __table_args__ = (
        PrimaryKeyConstraint("id"),
        UniqueConstraint("build_session_id", "platform_hash",
                         "compiler_idx", "operation_name", "primitive_name",
                         "implementation_hash"),
        ForeignKeyConstraint(["build_session_id"], ["build_session.id"]),
        ForeignKeyConstraint(["platform_hash"], ["platform.hash"]),
        ForeignKeyConstraint(["primitive_name", "operation_name"],
                             ["primitive.name", "primitive.operation_name"]),
        ForeignKeyConstraint(["operation_name"], ["operation.name"]),
        ForeignKeyConstraint(["implementation_hash"], ["implementation.hash"]),
        ForeignKeyConstraint(
            ["platform_hash", "compiler_idx"],
            ["compiler.platform_hash", "compiler.idx"]),
    )

    def __init__(self, build_session, compiler_idx, implementation,
            parallel_make=False, **kwargs):
        super().__init__(**kwargs)


        self.build_session  = build_session
        config              = build_session.config

        self.platform       = config.platform
        self.compiler       = config.platform.compilers[compiler_idx]
        self.compiler_idx   = compiler_idx

        self.implementation = implementation
        self.primitive      = self.implementation.primitive
        self.operation      = self.primitive.operation

        self.work_path      = os.path.join(
            config.platform.name,
            self.implementation.primitive.operation.name,
            self.implementation.primitive.name,
            self.implementation.name,
            str(compiler_idx)
        )
        self.exe_path       = os.path.join(self.work_path, EXE_NAME)
        self.hex_path       = os.path.join(self.work_path, HEX_NAME)

        self.parallel_make  = False

        self.text = None
        self.data = None
        self.bss  = None

        self.timestamp = None
        self.hex_checksum = None

        self.rebuilt = False

        if not self.platform.validate_hash(config.platforms_path):
            raise ValueError("Platform hash failed to verify")
        if not self.implementation.validate_hash(config.algopack_path):
            raise ValueError("Implementation hash failed to verify")

    def get_buildjob(self):
        """Returns instance of BuildJob"""
        config = self.build_session.config
        full_work_path = os.path.join(config.work_path, self.work_path)

        full_hex_path = os.path.join(config.work_path, self.hex_path)
        full_exe_path = os.path.join(config.work_path, self.exe_path)

        if os.path.isdir(full_work_path):
            self.rebuilt = True
            try:
                os.remove(full_hex_path)
                os.remove(full_exe_path)
            except FileNotFoundError:
                pass


        self._gen_files()

        return BuildJob(
            full_work_path, self.parallel_make,
            {'buildid': self.buildid},
            self.buildid, self.platform.name,
            full_hex_path
        )

    def do_postbuild(self, job):
        """Extracts data from completed buildjob

        Also inspects post build environment for stats
        """
        self.build_ok = False
        config = self.build_session.config
        full_hex_path = os.path.join(config.work_path, self.hex_path)
        full_exe_path = os.path.join(config.work_path, self.exe_path)

        if os.path.isfile(full_exe_path):
            size = os.path.join(config.platforms_path,
                                self.platform.path, 'size')
            total_env = self.env.copy()
            total_env.update(os.environ)
            stdout = subprocess.check_output((size, full_exe_path),
                    env=total_env)
            sizeout = stdout.decode().splitlines()[1]
            match = re.match(r'^\s*(\w+)\s+(\w+)\s+(\w+)', sizeout)

            self.text = int(match.group(1))
            self.data = int(match.group(2))
            self.bss  = int(match.group(3))

            if os.path.isfile(full_hex_path):
                self.hex_checksum = xbx.util.sha256_file(full_hex_path)
                self.build_ok = True

        self.timestamp = job.timestamp
        self.log = job.log


    @property
    def buildid(self):
        buildid = "{}/{}/{}/{}".format(
            self.operation.name,
            self.primitive.name,
            self.implementation.name,
            self.compiler_idx)
        return buildid


    @property
    def env(self):
        config = self.build_session.config

        dependencies = self.implementation.get_config_assoc(config).dependencies

        dep_paths = [os.path.abspath(os.path.join(config.algopack_path, i.path))
                     for i in dependencies]
        # Set build environment variables
        tmpl_path = ''
        if self.platform.tmpl_path:
            tmpl_path = os.path.join(config.platforms_path, self.platform.tmpl_path)
        env = {
            'templatePlatformDir': tmpl_path,
            'HAL_PATH':            os.path.join(config.platforms_path,
                                                self.platform.path,'hal'),
            'HAL_T_PATH':          os.path.join(tmpl_path,
                                                'hal') if tmpl_path else '',
            'XBD_PATH':            os.path.join(config.embedded_path,'xbd'),
            'IMPL_PATH':           os.path.join(config.algopack_path,
                                                self.implementation.path),
            'ALGOPACK_PATH':       config.algopack_path,
            'POSTLINK':            os.path.join(config.platforms_path,
                                                self.platform.path, 'postlink'),
            'HAL_OBJS':            os.path.join(
                self.build_session.config.work_path,
                self.build_session.config.platform.name,
                'HAL',
                str(self.compiler_idx)
            )
        }

        # Make paths absolute, except for compilers
        for k, v in env.items():
            # If not empty
            if v:
                env[k] = os.path.abspath(v)

        # Add compilers and non path env variables
        env.update({
            'OP': self.operation.name,
            'CC': self.compiler.cc, 'CXX': self.compiler.cxx,
            'DEP_PATHS': ' '.join(dep_paths)})
        return env


    def _gen_files(self):
        operation = self.implementation.primitive.operation
        primitive = self.implementation.primitive

        config = self.build_session.config
        full_work_path = os.path.join(config.work_path, self.work_path)

        # Generate files 
        try:
            os.makedirs(full_work_path)
        except OSError:
            pass

        _gen_envfile(full_work_path, self.env)

        makefile = os.path.join(full_work_path, 'Makefile')
        o_h = os.path.join(full_work_path, operation.name+".h")
        op_h = os.path.join(full_work_path,
                operation.name + "_" + primitive.name+".h")
        if not os.path.isfile(makefile):
            self._genmake(makefile)
        if not os.path.isfile(o_h):
            self._gen_o_h(o_h, primitive)
        if not os.path.isfile(op_h):
            self._gen_op_h(op_h, self.implementation)

        dependencies = self.implementation.get_config_assoc(config).dependencies

        # Generate dependency header files
        for i in dependencies:
            o_h = os.path.join(full_work_path, i.primitive.operation.name + ".h")
            op_h = os.path.join(full_work_path, i.primitive.operation.name +
                                '_' + i.primitive.name+ ".h")
            if not os.path.isfile(o_h):
                self._gen_o_h(o_h, i.primitive)
            if not os.path.isfile(op_h):
                self._gen_op_h(op_h, i)


    def validate_hex_checksum(self):
        """Verifies if checksum still valid"""
        config = self.build_session.config
        full_hex_path = os.path.join(config.work_path, self.hex_path)
        hash = xbx.util.sha256_file(full_hex_path)
        return hash == self.hex_checksum

    def _genmake(self, filename):
        import xbx.buildfiles as buildfiles
        #self.logger.debug("Generating Makefile...", extra=self.log_attr)
        with open(filename, 'w') as f:
            f.write(buildfiles.MAKEFILE)


    def _gen_o_h(self, filename, primitive):
        import xbx.buildfiles as buildfiles
        #self.logger.debug("Generating "+filename+"...", extra=self.log_attr)
        macro_expand = []
        o = primitive.operation.name
        op = o + '_'+primitive.name

        for m in primitive.operation.macro_names:
            macro_expand+=("#define {}{} {}{}".format( o, m, op, m),)
        macro_expand+=("#define {} {}".format(o, op),)

        subst_dict = {
                'op': op,
                'o': o,
                'p': primitive.name,
                'op_macros': '\n'.join(macro_expand)
                }

        with open(filename, 'w') as f:
            f.write(string.Template(buildfiles.O_H).substitute(subst_dict))


    def _gen_op_h(self, filename, implementation):
        config = self.build_session.config
        import xbx.buildfiles as buildfiles
        #self.logger.debug("Generating "+filename+"...", extra=self.log_attr)
        operation = implementation.primitive.operation
        primitive = implementation.primitive

        macro_expand = []
        prototype_expand = []
        o = primitive.operation.name
        op = o + '_'+primitive.name
        opi = op + '_' + implementation.name
        api_h = None

        for m in operation.macro_names:
            macro_expand+=("#define {}{} {}{}".format( op, m, opi, m),)
        macro_expand+=("#define {} {}".format(op, opi),)

        for pr in operation.prototypes:
            prototype_expand+=("extern int {}{};".format(opi, pr),)

        with open(os.path.join(config.algopack_path,
                               implementation.path, "api.h")) as f:
            api_h = f.read()
            api_h = re.sub(r"^#define .*("+'|'.join(map(re.escape,
                                                        implementation.macros.keys()))+")",
                           "#define "+opi+r"\1", api_h, flags=re.MULTILINE)

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
            f.write(string.Template(buildfiles.OP_H).substitute(subst_dict))


# Support fxns
def build_hal(config, index):
    """Builds HAL for given compiler index"""

    import xbx.buildfiles as buildfiles
    _logger.info("Building HAL for platform {}, compiler {}".format(
            config.platform.name,
            str(index)))


    work_path = os.path.join(
            config.work_path,
            config.platform.name,
            'HAL',
            str(index))

    tmpl_path = None
    if config.platform.tmpl_path:
        tmpl_path = os.path.join(config.platforms_path, config.platform.tmpl_path)

    try:
        os.makedirs(work_path)
    except OSError:
        pass

    makefile = os.path.join(work_path, 'Makefile')

    if not os.path.isfile(makefile):
        with open(makefile, 'w') as f:
            f.write(buildfiles.HAL_MAKEFILE)


    env = ({
        'templatePlatformDir': tmpl_path if tmpl_path else '',
        'XBD_PATH': os.path.join(config.embedded_path,'xbd'),
        'HAL_PATH': os.path.join(config.platforms_path,
                                 config.platform.path,'hal'),
        'HAL_T_PATH': os.path.join(tmpl_path,'hal') if tmpl_path else ''})

    for k, v in env.items():
        if v:
            env[k] = os.path.abspath(v)

    env.update({'CC': config.platform.compilers[index].cc})

    _gen_envfile(work_path, env)

    _make(work_path, _logger.debug, parallel = True)

def _make(path, log_fn, target="all", parallel=False, extra=[]):
    """Runs subprocess logging output

    Parameters:
    cmd         Command given to subprocess.Popen()
    path        Path to chdir before execution
    env         Environment for command
    log_fn      Logging function for stdout
    err_logfn   Logging function for stderr
    log_array   Array of strings to append output to
    extra       Extra to send to logger

    Return:
    Make output as string

    """

    cmd = ["make"]
    if parallel:
        cmd += "-j",
    cmd += target,

    old_pwd = os.getcwd()
    os.chdir(path)
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    os.chdir(old_pwd)

    stdout,_ = process.communicate()

    for i in stdout.decode().split("\n"):
        log_fn(i, extra=extra)

    return stdout

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


class BuildSession(Base, xbxs.SessionMixin):
    """Manages builds for all instances specified in xbx config

    Pass in database object into constructor to save session and populate id
    Database will not be committed until buildall() is initated and
    successfully completed
    """

    __tablename__ = "build_session"

    parallel = Column(Boolean)

    builds = relationship("Build", backref="build_session")


    def __init__(self, config, *args, **kwargs):
        super().__init__(config=config, *args, **kwargs)
        self._setup_session()
        self.parallel = config.parallel_build

    def buildall(self):
        """Builds all targets specified in xbx self.config

        If database was specified in constructor, completion will commit
        database entry.
        """
        self.cpu_count = mp.cpu_count()

        #logger = logging.getLogger(__name__)
        num_compilers = len(self.config.platform.compilers)
        build_map = {}

        for i in range(num_compilers):
            compiler = self.config.platform.compilers[i]
            _logger.info("compiler[{}] = {}".format(i, str((compiler.cc, compiler.cxx))))
            if self.config.one_compiler:
                break

        for i in range(num_compilers):
            build_hal(self.config, i)
            if self.config.one_compiler:
                break

        for j in self.config.implementations:
            for i in range(num_compilers):
                # Don't have to add to self.builds as Build sets
                # BuildSession as parent, which inserts into
                # BuildSession.builds
                b = Build(self, i, j)
                build_map[b.buildid] = b
                if self.config.one_compiler:
                    break

        if self.config.parallel_build:
            q_out = mp.Queue()
            q_in = mp.Queue()

            def worker(q_in, q_out, n):
                _logger.info("Worker "+str(n)+" started")
                for job in iter(q_in.get, None):
                    # Re-add this, since it gets lost in multiprocessing for
                    # some reason
                    job()
                    q_out.put(job)
                _logger.info("Worker "+str(n)+" finished")

            processes = [mp.Process(target=worker, args=(q_out,q_in, i))
                    for i in range(self.cpu_count+1)]
            for p in processes:
                p.start()
                # Terminate if ctrl-c
                atexit.register(p.terminate)

            for b in self.builds:
                job = b.get_buildjob()
                q_out.put(job)


            # Clear out old build list, reobtain from queue with updated data
            num_builds = len(self.builds)
            for _ in range(num_builds):
                job = q_in.get()
                build = build_map[job.buildid]
                build.do_postbuild(job)


            # Terminate processes gracefully
            for p in processes:
                q_out.put(None)
        else:
            for b in self.builds:
                job = b.get_buildjob()
                job()
                b.do_postbuild(job)


        self.timestamp = datetime.now()

    def __lt__(self, other):
        return self.session_id < other.session_id

