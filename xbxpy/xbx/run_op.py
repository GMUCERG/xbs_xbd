import logging
import struct
import sys
import os

from sqlalchemy.schema import ForeignKeyConstraint, PrimaryKeyConstraint
from sqlalchemy import Column, ForeignKey, Integer, String, Text, Boolean, Date
from sqlalchemy.orm import relationship
from sqlalchemy.ext.declarative import declarative_base

import xbx.run as xbxr
#from xbx.run import Run
import xbx.session
import xbh

logger = logging.getLogger(__name__)

# Creates another table that is joined to run to generate a Hashrun instance
class HashRun(xbx.run.Run):
    __tablename__ = "hash_run"

    id = Column(Integer)
    msg_len = Column(Integer)

    __mapper_args__ = {
        'polymorphic_identity':'hash_run',
    }

    __table_args__ = (
        PrimaryKeyConstraint("id"),
        ForeignKeyConstraint(["id"], ["run.id"]))

    def __init__(self, build_exec, params=None, **kwargs):
        super().__init__(build_exec, params,  **kwargs)
        self.msg_len = params[0]

    def _assemble_params(self):
        data = bytearray(struct.pack("!I",self.msg_len))
        data += os.urandom(self.msg_len)
        return data 
    
    def execute(self):
        super().execute()



class AeadRun(xbxr.Run):
    __tablename__ = "aead_run"

    id = Column(Integer)
    msg_len = Column(Integer)
    auth_data_len = Column(Integer)

    __mapper_args__ = {
        'polymorphic_identity':'aead_run',
    }

    __table_args__ = (
        PrimaryKeyConstraint("id"),
        ForeignKeyConstraint(["id"], ["run.id"]))

    def __init__(self, build_exec, params=None, **kwargs):
        super().__init__(build_exec, params,  **kwargs)
        self.msg_len = params[0]
        self.auth_data_len = params[1]

    def _assemble_params(self):
        data = bytearray(struct.pack("!I",self.msg_len))
        data += os.urandom(self.msg_len)
        data += bytearray(struct.pack("!I",self.auth_data_len))
        data += os.urandom(self.auth_data_len)
        return data 
    
    def execute(self):
        super().execute()


OPERATIONS={
    "crypto_aead": (AeadRun, xbh.TypeCode.AEAD),
    "crypto_hash": (HashRun, xbh.TypeCode.HASH),
}

