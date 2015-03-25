import logging
import datetime
import os
import inspect
import yaml


from sqlalchemy.schema import ForeignKeyConstraint, PrimaryKeyConstraint
from sqlalchemy import Column, ForeignKey, Integer, String, Text, Boolean, Date
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import relationship, sessionmaker
from sqlalchemy import create_engine


_logger = logging.getLogger(__name__)

Base = declarative_base()

#class RunSession(Base):
#    __tablename__ = "run_session"
#
#    id = Column(Integer)
#    timestamp = Column(Date)
#    host = Column(String)
#    xbx_version = Column(String)
#    build_session = Column(Integer)
#    config = Column(String)
#    xbh_rev = Column(String)
#    xbd_bl_rev = Column(String)
#
#    __table_args__ = (
#        PrimaryKeyConstraint("id"),
#        ForeignKeyConstraint(
#            ["config"],
#            ["config.hash"]),
#        ForeignKeyConstraint(
#            ["build_session"],
#            ["build_session.id"]),
#    )
#
#class DriftMeasurement(Base):
#    __tablename__ = "drift_measurement"
#
#    id = Column(Integer)
#    abs_error = Column(Integer)
#    rel_error = Column(Integer)
#    cycles = Column(Integer)
#    measured_cycles = Column(Integer)
#    run_session = Column(Integer)
#
#    __table_args__ = (
#        PrimaryKeyConstraint("id"),
#        ForeignKeyConstraint( ["run_session"], ["run_session.id"]),
#    )
#
#class Run(Base):
#    __tablename__ = "run"
#
#    id = Column(Integer)
#
#    measured_cycles = Column(Integer)
#    reported_cycles = Column(Integer)
#    time_ns = Column(Integer)
#    stackUsage = Column(Integer)
#
#    min_power = Column(Integer)
#    max_power = Column(Integer)
#    avg_power = Column(Integer)
#    median_power = Column(Integer)
#
#    power_data = Column(Text)
#    timestamp = Column(Date)
#    build = Column(Integer)
#    run_session = Column(Integer)
#
#    type = Column(String)
#
#    __table_args__ = (
#        PrimaryKeyConstraint("id"),
#        ForeignKeyConstraint( ["run_session"], ["run_session.id"]),
#    )
#
#    __mapper_args__ = {
#        'polymorphic_identity':'run',
#        'polymorphic_on':type ,
#    }
#
#
## Creates another table that is joined to run to generate a Hashrun instance
#class HashRun(Run):
#    __tablename__ = "hash_run"
#
#    id = Column(Integer)
#    input_len = Column(Integer)
#
#    __mapper_args__ = {
#        'polymorphic_identity':'hash_run',
#    }
#
#    __table_args__ = (
#        PrimaryKeyConstraint("id"),
#        ForeignKeyConstraint(["id"], ["run.id"]))
#
#class AeadRun(Run):
#    __tablename__ = "aead_run"
#
#    id = Column(Integer)
#    auth_data_len = Column(Integer)
#    secret_len = Column(Integer)
#
#    __mapper_args__ = {
#        'polymorphic_identity':'aead_run',
#    }
#
#    __table_args__ = (
#        PrimaryKeyConstraint("id"),
#        ForeignKeyConstraint(["id"], ["run.id"]))
#
#
#
#
#
from sqlalchemy.orm import scoped_session as ss, sessionmaker as sm
scoped_session = ss(sm())
def init(data_path):
    import xbx.config as xbxc
    global scoped_session
    engine = None
    if not os.path.isfile(data_path):
        _logger.info("Database doesn't exist, initializing")
        engine = create_engine('sqlite:///'+ data_path)
        Base.metadata.create_all(engine)
    else:
        engine = create_engine('sqlite:///'+ data_path)

    scoped_session.configure(bind=engine)


# From https://bitbucket.org/zzzeek/sqlalchemy/wiki/UsageRecipes/UniqueObject
def _unique(session, cls, hashfunc, queryfunc, constructor, arg, kw):
    cache = getattr(session, '_unique_cache', None)
    if cache is None:
        session._unique_cache = cache = {}

    key = (cls, hashfunc(*arg, **kw))
    if key in cache:
        return cache[key]
    else:
        with session.no_autoflush:
            q = session.query(cls)
            q = queryfunc(q, *arg, **kw)
            obj = q.first()
            if not obj:
                obj = constructor(*arg, **kw)
                session.add(obj)
        cache[key] = obj
        return obj

def unique_constructor(scoped_session, hashfunc, queryfunc):
    def decorate(cls):
        def _null_init(self, *arg, **kw):
            pass
        def __new__(cls, bases, *arg, **kw):
            # no-op __new__(), called
            # by the loading procedure
            if not arg and not kw:
                return object.__new__(cls)

            session = scoped_session()

            def constructor(*arg, **kw):
                obj = object.__new__(cls)
                obj._init(*arg, **kw)
                return obj

            return _unique(
                        session,
                        cls,
                        hashfunc,
                        queryfunc,
                        constructor,
                        arg, kw
                   )

        # note: cls must be already mapped for this part to work
        cls._init = cls.__init__
        cls.__init__ = _null_init
        cls.__new__ = classmethod(__new__)
        return cls

    return decorate





from sqlalchemy.types import TypeDecorator 
import json

class JSONEncodedDict(TypeDecorator):
    """Represents an immutable structure as a json-encoded string.

    Usage::

        JSONEncodedDict(255)

    """

    impl = Text

    def process_bind_param(self, value, dialect):
        if value is not None:
            value = json.dumps(value)

        return value

    def process_result_value(self, value, dialect):
        if value is not None:
            value = json.loads(value)
        return value






