import logging
import sys

from sqlalchemy.schema import ForeignKeyConstraint, PrimaryKeyConstraint
from sqlalchemy import Column, ForeignKey, Integer, String, Text, Boolean, DateTime
from sqlalchemy.orm import relationship
from sqlalchemy.ext.declarative import declarative_base

import xbx.database as xbxdb
import xbx.build as xbxb
import xbx.session as xbxs
import xbh

from xbx.database import Base
#from xbx.run_op import *

logger = logging.getLogger(__name__)


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

    power_data      = Column(Text)
    timestamp       = Column(DateTime)
    build           = Column(Integer)
    run_session_id     = Column(Integer)

    type            = Column(String)



    __table_args__ = (
        PrimaryKeyConstraint("id"),
        ForeignKeyConstraint( ["run_session_id"], ["run_session.id"]),
    )

    __mapper_args__ = {
        'polymorphic_identity':'run',
        'polymorphic_on': type ,
    }

    def __init__(self, run_session, **kwargs):
        super().__init__(**kwargs)
        self.run_session = run_session

    def execute(self):
        pass


class RunSession(Base, xbxs.SessionMixin):
    """Manages runs specified in config that are defined in the last
    BuildSession

    Pass in database object into constructor to save session and
    populate id 
    
    """
    __tablename__ = "run_session"

    xbh_rev            = Column(String)
    build_session_id   = Column(Integer)

    drift_measurements = relationship("DriftMeasurement", backref="run_session")
    runs               = relationship("Run", backref="run_session")

    build_session      = relationship("BuildSession", backref="run_sessions")

    __table_args__ = (
        ForeignKeyConstraint(
            ["build_session_id"],
            ["build_session.id"]),
    )

    def __init__(self, config, **kwargs):
        super().__init__(config=config, **kwargs)
        self._setup_session()

        s = xbxdb.scoped_session()
        q = s.query(xbxb.BuildSession).order_by(
            xbxb.BuildSession.timestamp.desc()).limit(1)
        self.build_session = q.one()



    def init_xbh(self):
        c = self.config
        try:
            self.xbh = xbh.Xbh(
                    c.xbh_addr, c.xbh_port, 
                    c.platform.pagesize,
                    c.platform.clock_hz)
        except xbh.XbhError as e:
            logger.critical(str(e))
            sys.exit(1)

        for i in range(10):
            self.drift_measurement()

        s = xbxdb.scoped_session()
        s.add(self)
        s.commit()

    @xbh.attempt("xbh", raise_err=True)
    def drift_measurement(self):
        (abs_error, 
         rel_error, 
         cycles, 
         measured_cycles) = self.xbh.timing_error_calc()  

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
        pass



class DriftMeasurement(Base):
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



        


