import logging
import datetime
import sqlite3
import hashlib
import socket
import os
import yaml


from sqlalchemy.schema import ForeignKeyConstraint, PrimaryKeyConstraint
from sqlalchemy import Column, ForeignKey, Integer, String, Text, Boolean, Date
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import relationship
from sqlalchemy import create_engine


import xbx.build as xbxb
import xbx.config as xbxc
import xbx.util as xbxu

_logger = logging.getLogger(__name__)

Base = declarative_base()

# Mapping objects
# {{{
class Platform(Base):
    __tablename__ = "platform"

    name = Column(String)
    clock_hz = Column(Integer)
    pagesize = Column(Integer)

    __table_args__ = (
        PrimaryKeyConstraint("name"),
    )

class Operation(Base):
    __tablename__ = "operation"

    name = Column(String)

    __table_args__ = (
        PrimaryKeyConstraint("name"),
    )

class Primitive(Base):
    __tablename__ = "primitive"

    operation = Column(String)
    name = Column(String)
    checksumsmall = Column(String)
    checksumlarge = Column(String)
    
    __table_args__ = (
        PrimaryKeyConstraint("operation", "name"),
        ForeignKeyConstraint(
            ["operation"],
            ["operation.name"]),
    )

class Config(Base):
    __tablename__ = "config"

    hash = Column(String)
    dump = Column(Text)

    __table_args__ = (
        PrimaryKeyConstraint("hash"),
    )


class BuildSession(Base):
    __tablename__ = "build_session" 

    id = Column(Integer)
    timestamp = Column(Date)
    host = Column(String)
    xbx_version = Column(String)
    parallel = Column(Boolean)
    config = Column(String)

    __table_args__ = (
        PrimaryKeyConstraint("id"),
        ForeignKeyConstraint(
            ["config"],
            ["config.hash"]),
    )
    

class Compiler(Base):
    __tablename__ = "compiler"
    build_session = Column(Integer)
    platform = Column(String, ForeignKey("platform.name"))
    idx = Column(Integer)

    cc_version = Column(String)
    cxx_version = Column(String)
    cc_version_full = Column(String)

    cxx_version_full = Column(String)
    cc = Column(String)
    cxx = Column(String)
    __table_args__ = (
        PrimaryKeyConstraint("build_session", "platform", "idx"),
        ForeignKeyConstraint(["build_session"], ["build_session.id"]),
        ForeignKeyConstraint(["platform"], ["platform.name"]),
    )


class Build(Base):
    __tablename__ = "build"
    
    id = Column(Integer)
    platform = Column(String)
    operation = Column(String) 
    primitive = Column(String) 
    implementation = Column(String) 
    impl_checksum = Column(String)
    compiler_idx = Column(Integer) 

    exe_path = Column(String)
    hex_path = Column(String)
    parallel_make = Column(Boolean)

    text = Column(Integer)
    data = Column(Integer)
    bss = Column(Integer)

    timestamp = Column(Date)
    hex_checksum = Column(String)
    build_session = Column(Integer)

    rebuilt = Column(Boolean)

    checksumsmall_result = Column(String) 
    checksumlarge_result = Column(String) 

    test_ok = Column(Boolean) 

    __table_args__ = (
        PrimaryKeyConstraint("id"),
        ForeignKeyConstraint(
            ["build_session"],
            ["build_session.id"]),
        ForeignKeyConstraint(
            ["platform", "build_session", "compiler_idx"],
            ["compiler.platform", "compiler.build_session", "compiler.idx"]),
    )

class RunSession(Base):
    __tablename__ = "run_session"

    id = Column(Integer)
    timestamp = Column(Date)
    host = Column(String)
    xbx_version = Column(String)
    build_session = Column(Integer)
    config = Column(String)
    xbh_rev = Column(String)
    xbd_bl_rev = Column(String)

    __table_args__ = (
        PrimaryKeyConstraint("id"),
        ForeignKeyConstraint(
            ["config"],
            ["config.hash"]),
        ForeignKeyConstraint(
            ["build_session"],
            ["build_session.id"]),
    )

class DriftMeasurement(Base):
    __tablename__ = "drift_measurement"

    id = Column(Integer)
    abs_error = Column(Integer)
    rel_error = Column(Integer)
    cycles = Column(Integer)
    measured_cycles = Column(Integer)
    run_session = Column(Integer)

    __table_args__ = (
        PrimaryKeyConstraint("id"),
        ForeignKeyConstraint( ["run_session"], ["run_session.id"]),
    )

class Run(Base):
    __tablename__ = "run"

    id = Column(Integer)

    measured_cycles = Column(Integer)
    reported_cycles = Column(Integer)
    time_ns = Column(Integer)
    stackUsage = Column(Integer)

    min_power = Column(Integer)
    max_power = Column(Integer)
    avg_power = Column(Integer)
    median_power = Column(Integer)

    power_data = Column(Text)
    timestamp = Column(Date)
    build = Column(Integer)
    run_session = Column(Integer)

    type = Column(String)

    __table_args__ = (
        PrimaryKeyConstraint("id"),
        ForeignKeyConstraint( ["run_session"], ["run_session.id"]),
    )

    __mapper_args__ = {
        'polymorphic_identity':'run',
        'polymorphic_on':type ,
    }


# Creates another table that is joined to run to generate a Hashrun instance
class HashRun(Run):
    __tablename__ = "hash_run"

    id = Column(Integer)
    input_len = Column(Integer)

    __mapper_args__ = {
        'polymorphic_identity':'hash_run',
    }

    __table_args__ = (
        PrimaryKeyConstraint("id"),
        ForeignKeyConstraint(["id"], ["run.id"]))

class AeadRun(Run):
    __tablename__ = "aead_run"

    id = Column(Integer)
    auth_data_len = Column(Integer)
    secret_len = Column(Integer)

    __mapper_args__ = {
        'polymorphic_identity':'aead_run',
    }

    __table_args__ = (
        PrimaryKeyConstraint("id"),
        ForeignKeyConstraint(["id"], ["run.id"]))
# }}}

class Database:

    def __init__(self, config):
        self.config = config
        self.engine = None
        if not os.path.isfile(config.data_path):
            _logger.info("Database doesn't exist, initializing")
            self.engine = create_engine('sqlite:///'+ config.data_path)
            Base.metadata.create_all(self.engine)

        else:
            self.engine = create_engine('sqlite:///'+ config.data_path)











