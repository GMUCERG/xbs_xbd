import logging
import sys
from datetime import datetime

from sqlalchemy.schema import ForeignKeyConstraint, PrimaryKeyConstraint, UniqueConstraint
from sqlalchemy import Column, ForeignKey, Integer, String, Text, Boolean, DateTime
from sqlalchemy.orm import relationship, reconstructor
from sqlalchemy.orm.exc import NoResultFound 
from sqlalchemy.ext.declarative import declarative_base

from xbx.database import JSONEncodedDict
from xbx.database import Base, unique_constructor, scoped_session
import xbx.database as xbxdb
import xbx.build as xbxb
import xbx.session as xbxs
import xbh as xbhlib

_logger = logging.getLogger(__name__)

class Error(Exception):
    pass

class NoBuildSessionError(Error):
    pass

class XbdChecksumFailError(Error, ValueError):
    pass


class Run(Base):
    __tablename__   = "run"

    id              = Column(Integer)

    measured_cycles = Column(Integer)
    reported_cycles = Column(Integer)
    time_ns         = Column(Integer)
    stackUsage      = Column(Integer)

    min_power       = Column(Integer)
    max_power       = Column(Integer)
    avg_power       = Column(Integer)
    median_power    = Column(Integer)

    power_data      = Column(JSONEncodedDict)
    timestamp       = Column(DateTime)
    build_exec_id   = Column(Integer)

    type            = Column(String)

    __table_args__ = (
        PrimaryKeyConstraint("id"),
        ForeignKeyConstraint( ["build_exec_id"], ["build_exec.id"]),
    )

    __mapper_args__ = {
        'polymorphic_identity':'run',
        'polymorphic_on': type ,
    }

    def __init__(self, build_exec, params=None, **kwargs):
        super().__init__(**kwargs)
        self.build_exec = build_exec
        self.params = params

        self.xbh = self.build_exec.xbh

    @reconstructor
    def __load_init(self):
        pass

    def _assemble_params(self):
        pass

    def _calculate_power(self):
        pass
    
    def execute(self):
        params = self.params
        _logger.info("Running build {} with params {}".
                     format(self.build_exec.build,self.params ))
        import xbx.run_op 
        operation_name = self.build_exec.run_session.config.operation.name
        runtype, typecode = xbx.run_op.OPERATIONS[operation_name]

        self.xbh.upload_param(self._assemble_params(), typecode)
        self.xbh.exec_and_time()
        self.measured_cycles = self.xbh.get_measured_cycles()
        self.timestamp = datetime.now()



class TestRun(Run):
    __tablename__ = "test_run"

    id = Column(Integer)
    checksumsmall_result = Column(String)
    checksumlarge_result = Column(String)
    test_ok              = Column(Boolean)

    __mapper_args__ = {
        'polymorphic_identity':'test_run',
    }

    __table_args__ = (
        PrimaryKeyConstraint("id"),
        ForeignKeyConstraint(["id"], ["run.id"]))

    def execute(self):
        self.xbh.calc_checksum()
        retval, self.checksumsmall_result = self.xbh.get_results()
        retcode = int(retval[0:2])
        if retcode != 0:
            _logger.error("Checksum failed with return code {}".format(retcode))
            self.test_ok = False
        elif (self.checksumsmall_result ==
              self.build_exec.build.primitive.checksumsmall): 
            self.test_ok = True
        else:
            self.test_ok = False

        self.measured_cycles = self.xbh.get_measured_cycles()
        self.timestamp = datetime.now()




@unique_constructor(
    scoped_session, 
    lambda run_session, build, *args, **kwargs: 
        str(run_session.id)+"_"+str(build.id), 
    lambda query, run_session, build, *args, **kwargs: 
        query.filter(BuildExec.build==build, BuildExec.run_session==run_session)
)
class BuildExec(Base):
    __tablename__  = "build_exec"
    id             = Column(Integer)

    build_id       = Column(Integer)
    run_session_id = Column(Integer)

    build          = relationship("Build")
    runs           = relationship("Run", backref="build_exec",
                                  cascade="all,delete-orphan")
    #primaryjoin="and_(Run.build_exec_id==BuildExec.id," "Run.type!='test_run')")
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
        op_name = self.run_session.config.operation.name
        self.RunType, typecode = xbx.run_op.OPERATIONS[op_name]


    @xbhlib.attempt("xbh", tries=2, raise_err=True)
    def load_build(self):
        self.xbh.upload_prog(self.build.hex_path)


    @xbhlib.attempt("xbh")
    def execute(self):
        # If not already run w/ a pass/fail
        if self.test_ok == None:
            del self.runs[:] # Delete existing runs for this build
            config = self.run_session.config
            num_start_tests = config.checksum_tests//2
            num_end_tests = config.checksum_tests-num_start_tests
            _logger.info("Testing build {}".format(self.build))
            def test(num):
                _logger.info("Calculating Checksum...")
                for i in range(num):
                    t = TestRun(self)
                    t.execute()
                    if not t.test_ok:
                        raise XbdChecksumFailError(t)
            try:
                # Test for half specified runs before running benchmarks
                test(num_start_tests)

                for i in range(config.exec_runs):
                    for p in self.run_session.config.operation_params:
                        r = self.RunType(self, p)
                        r.execute()

                # Test for remaining specified runs after running benchmarks to see
                # if results still valid to check if not corrupted
                test(num_end_tests)
            except XbdChecksumFailError as ve:
                self.test_ok = False
                _logger.error("Build " + str(self.build) + " fails checksum tests")
            else:
                self.test_ok = True
            




    

class RunSession(Base, xbxs.SessionMixin):
    """Manages runs specified in config that are defined in the last
    BuildSession

    Pass in database object into constructor to save session and
    populate id 
    
    """
    __tablename__ = "run_session"

    xbh_rev            = Column(String)
    xbh_mac            = Column(String)
    build_session_id   = Column(Integer)

    drift_measurements = relationship("DriftMeasurement", backref="run_session")
    build_execs        = relationship("BuildExec", backref="run_session")

    build_session      = relationship("BuildSession", backref="run_sessions")

    __table_args__ = (
        ForeignKeyConstraint( ["build_session_id"], ["build_session.id"]),
    )

    def __init__(self, config, build_session_id=None, *args, **kwargs):
        super().__init__(config=config, *args, **kwargs)
        self._setup_session()

        try: 
            if build_session_id == None:
                s = xbxdb.scoped_session()
                q = s.query(xbxb.BuildSession).order_by(
                    xbxb.BuildSession.timestamp.desc()).limit(1)
                self.build_session = q.one()
            else:
                # Setting this also sets self.build_session
                self.build_session_id = build_session_id
        except NoResultFound as e:
            raise NoBuildSessionError("Build must be run first") from e




    def init_xbh(self):
        c = self.config
        try:
            self.xbh = xbhlib.Xbh(
                    c.xbh_addr, c.xbh_port, 
                    c.platform.pagesize,
                    c.platform.clock_hz)
        except xbhlib.Error as e:
            logger.critical(str(e))
            sys.exit(1)

        self.xbh_rev,self.xbh_mac = self.xbh.get_xbh_rev()

        # Initialize xbh attribute on all build_execs and build_exec.runs if items loaded
        # from db
        for be in self.build_execs:
            be.xbh = self.xbh
            for r in be.runs:
                r.xbh = self.xbh

        s = xbxdb.scoped_session()
        s.add(self)
        s.commit()


    @xbhlib.attempt("xbh", raise_err=True)
    def do_drift_measurement(self):
        for i in range(self.config.drift_measurements):
            (abs_error, 
             rel_error, 
             cycles, 
             measured_cycles) = self.xbh.measure_timing_error()  

            logger.debug(("Abs Error: {}, Rel Error: {}, Cycles: {}, "
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
        for b in self.build_session.builds:
            be = BuildExec(self,b)
            be.load_build()
            be.execute()


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
