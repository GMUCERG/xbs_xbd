import binascii
import logging
import socket
import sys
import binascii
import os
from datetime import datetime
import time

from sqlalchemy.schema import ForeignKeyConstraint, PrimaryKeyConstraint, UniqueConstraint
from sqlalchemy import Column, ForeignKey, Integer, String, Text, Boolean, DateTime, Float 
from sqlalchemy.orm import relationship, reconstructor
from sqlalchemy.orm.exc import NoResultFound
from sqlalchemy.ext.declarative import declarative_base

from xbx.database import JSONEncodedDict
from xbx.database import Base, unique_constructor, scoped_session
import xbx.config as xbxc
import xbx.database as xbxdb
import xbx.build as xbxb
import xbx.session as xbxs
import xbh as xbhlib

_logger = logging.getLogger(__name__)

class Error(Exception):
    pass

class NoBuildSessionError(Error):
    pass

class RunValueError(Error):
    pass

class XbdResultFailError(RunValueError):
    pass


class PowerSample(Base):
    __tablename__ = "power_sample"
    id      = Column(Integer, nullable=False)
    run_id  = Column(Integer, nullable=False)
    power   = Column(Float)
    current = Column(Float)
    voltage = Column(Float)
    timestamp = Column(Integer)

    __table_args__ = (
        PrimaryKeyConstraint("id"),
        ForeignKeyConstraint( ["run_id"], ["run.id"]),
    )



class Run(Base):
    """Contains data generic to all runs

    Power values are summarized for easy analysis and sorting, instead of adding
    each sample as a row.

    This class should not be instantiated directly. Call the run() class method
    of subclasses

    Attributes of interest:

        measured_cycles     Cycles calculated from measured time and known clock
                            rate

        reported_cycles     Cycles reported by cycle counter if applicable

        time                Time taken for run in nanoseconds

        stack_usage         Stack usage in bytes

        min_power           Min power

        max_power           Max power

        avg_power           Average power

        median_power        Median power

        total_energy        Total energy

        power_data          JSON encoded power data

        timestamp           Timestamp of execution copmletion

    """
    __tablename__   = "run"

    id              = Column(Integer, nullable=False)

    measured_cycles = Column(Integer)
    reported_cycles = Column(Integer)
    time            = Column(Integer)
    stack_usage     = Column(Integer)

    min_power       = Column(Integer)
    max_power       = Column(Integer)
    avg_power       = Column(Integer)
    median_power    = Column(Integer)
    total_energy    = Column(Integer)

    power_samples   = relationship("PowerSample", backref="run")
    timestamp       = Column(DateTime)
    build_exec_id   = Column(Integer)

    run_type        = Column(String, nullable=False)

    __table_args__ = (
        PrimaryKeyConstraint("id"),
        ForeignKeyConstraint( ["build_exec_id"], ["build_exec.id"]),
    )

    __mapper_args__ = {
        'polymorphic_identity':'run',
        'polymorphic_on': run_type,
    }

    def __init__(self, build_exec, **kwargs):
        super().__init__(**kwargs)
        self.build_exec = build_exec

    def _assemble_params(self):
        """Extend this to return parameters packed to send to XBD"""
        pass

    def _calculate_power(self):
        s = xbxdb.scoped_session()
        volts = self.xbh.get_power();
        pwrSample = PowerSample(voltage=volts, run=self)
        s.add(pwrSample)
        s.commit()

    def _execute(self, packed_params=None):
        """Executes and returns results

        packed_params is an optional bytearray containing parameters to pass to
        XBD. Format of packed parameters is defined by XBD code
        """
        import xbx.run_op

        xbh = self.build_exec.run_session.xbh

        operation_name = self.build_exec.build.operation.name
        runtype, typecode = xbx.run_op.OPERATIONS[operation_name]
        if packed_params != None:
            xbh.upload_param(packed_params, typecode)
        results, timings, self.stack_usage = xbh.execute()
        self.timestamp = datetime.now()

        self.measured_cycles = xbh.get_measured_cycles(timings)
        self.time = xbh.get_measured_time(timings)

        return results

    @classmethod
    def run(cls, build_exec, params=None):
        """Does nothing, override this to instantiate run(s), execute, and
        return"""
        pass
        ## Do nothing, let subclass handle this


class TestRun(Run):
    __tablename__ = "test_run"

    id = Column(Integer, nullable=False)
    checksumsmall_result = Column(String)
    checksumfail_cause   = Column(String)
    test_ok              = Column(Boolean)

    __mapper_args__ = {
        'polymorphic_identity':'test_run',
    }

    __table_args__ = (
        PrimaryKeyConstraint("id"),
        ForeignKeyConstraint(["id"], ["run.id"]))

    def _execute(self, packed_params=None):
        self.test_ok = False
        xbh = self.build_exec.run_session.xbh
        results = None
        timings = None
        try:
            results, timings, self.stack_usage = xbh.calc_checksum()
        except xbhlib.ChecksumFailError as e:
            self.checksumfail_cause = str(e)
            raise e

        self.timestamp = datetime.now()
        self.measured_cycles = xbh.get_measured_cycles(timings)
        self.time = xbh.get_measured_time(timings)


        retval, data = results
        self.checksumsmall_result = binascii.hexlify(data).decode()
        if retval != 0:
            _logger.error("Checksum failed with return code {}".format(retval))
            self.test_ok = False
        elif (self.checksumsmall_result ==
              self.build_exec.build.primitive.checksumsmall):
            self.test_ok = True



    @classmethod
    def run(cls, build_exec, params=None):
        """Factory method that generates run instances and attaches them to
        buildexec. Call this instead of constructor"""
        _logger.info("Running tests on {}".
                     format(build_exec.build.buildid))
        run = cls(build_exec)
        run._execute()
        return run


# Decorator to pull BuildExec out of database if instance already exists for
# given Build and this run_session
@unique_constructor(
    scoped_session,
    lambda run_session, build, *args, **kwargs:
        str(run_session.id)+"_"+str(build.id),
    lambda query, run_session, build, *args, **kwargs:
        query.filter(BuildExec.build==build, BuildExec.run_session==run_session)
)
class BuildExec(Base):
    __tablename__  = "build_exec"
    id             = Column(Integer, nullable=False)

    build_id       = Column(Integer, nullable=False)
    run_session_id = Column(Integer, nullable=False)

    build          = relationship("Build")
    runs           = relationship("Run", backref="build_exec",
                                  cascade="all,delete-orphan")
    test_ok        = Column(Boolean)

    __table_args__ = (
        PrimaryKeyConstraint("id"),
        UniqueConstraint("run_session_id", "build_id"),
        ForeignKeyConstraint(["build_id"], ["build.id"]),
        ForeignKeyConstraint(["run_session_id"], ["run_session.id"]),
    )

    def __init__(self, run_session, build, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.build = build
        self.run_session = run_session
        self.xbh = run_session.xbh
        self.__load_init()


    @reconstructor
    def __load_init(self):
        import xbx.run_op
        op_name = self.build.operation.name
        self.RunType, typecode = xbx.run_op.OPERATIONS[op_name]


    @xbhlib.attempt("xbh", tries=2, raise_err=True)
    def load_build(self):
        _logger.info("Loading build {} into XBD".
                     format(self.build.buildid))

        if not self.build.validate_hex_checksum():
            raise RunValueError("Hex checksum for build invalid. Rerun build")

        full_hex_path = os.path.join(self.run_session.config.work_path,
                                     self.build.hex_path)
        self.xbh.upload_prog(full_hex_path)

    def execute(self):
        del self.runs[:] # Delete existing runs for this build
        config = self.run_session.config

        # Make sure num_start_tests is the bigger half of config.checksum_tests//2
        num_end_tests = config.checksum_tests//2
        num_start_tests = config.checksum_tests-num_end_tests
        def test(num):
            if not self.build.primitive.checksumsmall:
                _logger.warn("No checksum for build {}".format(self.build.buildid))
                return

            for i in range(num):
                _logger.info("Testing build {}".format(self.build.buildid))

                runner = xbhlib.attempt(self.xbh)(TestRun.run)
                t = runner(self)
                if not t.test_ok:
                    raise XbdResultFailError("Build " + str(self.build.buildid) +
                                               " fails checksum tests")

        try:
            _logger.info("Running benchmarks for {}".format(self.build.buildid))
            # Test for half specified runs before running benchmarks
            test(num_start_tests)

            for i in range(config.exec_runs):
                for p in self.run_session.config.operation_params:
                    runner = xbhlib.attempt(self.xbh)(self.RunType.run)
                    runner(self, p)

            # Test for remaining specified runs after running benchmarks to see
            # if results still valid to check if not corrupted
            test(num_end_tests)
        except (xbhlib.ChecksumFailError,RunValueError) as e:
            self.test_ok = False
            _logger.error(str(e))
        else:
            self.test_ok = True





# Gets build session, or latest if none is provided
def _get_build_session(config, build_session_id=None):
    try:
        s = xbxdb.scoped_session()
        if build_session_id == None:
            return (s.query(xbxb.BuildSession).join("config","operation").
                    filter(xbxc.Operation.name == config.operation.name).
                    order_by(xbxb.BuildSession.timestamp.desc())).first()
        else:
            return s.query(xbxb.BuildSession).filter(BuildSession.id == build_session_id).one()
    except NoResultFound as e:
        raise NoBuildSessionError("Build must be run first") from e



# Decorator to pull RunSession out of database if instance already exists for
# given config
@unique_constructor(
    scoped_session,
    lambda config, build_session_id=None, *args, **kwargs:
        (str(config.hash)+"_"+
         str(_get_build_session(config, build_session_id).id)),
    lambda query, config, build_session_id=None, *args, **kwargs:
        query.filter(RunSession.config_hash==config.hash,
                     RunSession.host==socket.gethostname(),
                     RunSession.build_session_id==
                     _get_build_session(config, build_session_id).id)
)
class RunSession(Base, xbxs.SessionMixin):
    """Manages runs specified in config that are defined in the last
    BuildSession

    Pass in database object into constructor to save session and
    populate id

    """
    __tablename__ = "run_session"

    xbh_rev            = Column(String)
    xbh_mac            = Column(String)
    build_session_id   = Column(Integer, nullable=False)

    drift_measurements = relationship("DriftMeasurement", backref="run_session")
    build_execs        = relationship("BuildExec", backref="run_session")

    build_session      = relationship("BuildSession", backref="run_sessions")

    __table_args__ = (
        ForeignKeyConstraint( ["build_session_id"], ["build_session.id"]),
    )

    def __init__(self, config, build_session_id=None, *args, **kwargs):
        super().__init__(config=config, *args, **kwargs)
        self._setup_session()
        self.build_session = _get_build_session(config, build_session_id)


    @reconstructor
    def __load_init(self):
        pass


    def init_xbh(self):
        c = self.config
        s = xbxdb.scoped_session()
        try:
            self.xbh = xbhlib.Xbh(
                c.xbh_addr, c.xbh_port,
                c.platform.pagesize,
                c.platform.clock_hz,
                c.xbh_timeout
            )

        except xbhlib.Error as e:
            logger.critical(str(e))
            sys.exit(1)

        self.xbh_rev,self.xbh_mac = self.xbh.get_xbh_rev()

        if not self.drift_measurements:
            self.do_drift_measurement()

        s.add(self)
        s.commit()
        # Initialize xbh attribute on all build_execs and build_exec.runs if items loaded
        # from db
        for be in self.build_execs:
            be.xbh = self.xbh


    @xbhlib.attempt("xbh", raise_err=True)
    def do_drift_measurement(self):
        _logger.info("Doing drift measurement...")
        for i in range(self.config.drift_measurements):

            (abs_error,
             rel_error,
             cycles,
             measured_cycles) = self.xbh.measure_timing_error()

            _logger.info(("Abs Error: {}, Rel Error: {}, Cycles: {}, "
                          "Measured Cycles: {}").format(abs_error,
                                                       rel_error,
                                                       cycles,
                                                       measured_cycles))

            DriftMeasurement(
                abs_error=abs_error,
                rel_error=rel_error,
                cycles=cycles,
                measured_cycles=measured_cycles,
                run_session=self
            )

            

    def runall(self):
        for b in (i for i in self.build_session.builds if i.build_ok):
            s = xbxdb.scoped_session()
            be = BuildExec(self,b)

            # If not already run w/ a pass/fail, or if rerunning results is
            # enabled, then do run
            if be.test_ok == None or self.config.rerun:
                be.load_build()
                be.execute()
                s.add(be)
                s.commit()


class DriftMeasurement(Base):# {{{
    __tablename__ = "drift_measurement"

    id = Column(Integer)
    abs_error = Column(Integer)
    rel_error = Column(Integer)
    cycles = Column(Integer)
    measured_cycles = Column(Integer)
    run_session_id = Column(Integer)

    __table_args__ = (
        PrimaryKeyConstraint("id"),
        ForeignKeyConstraint( ["run_session_id"], ["run_session.id"]),
    )
# }}}
