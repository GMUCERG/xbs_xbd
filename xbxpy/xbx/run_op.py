import logging
import struct
import sys
import os

from sqlalchemy.schema import ForeignKeyConstraint, PrimaryKeyConstraint
from sqlalchemy import Column, ForeignKey, Integer, String, Text, Boolean, Date, Enum
from sqlalchemy.orm import relationship
from sqlalchemy.ext.declarative import declarative_base

import xbx.run as xbxr
#from xbx.run import Run
import xbx.session
import xbh

_logger = logging.getLogger(__name__)

# Creates another table that is joined to run to generate a Hashrun instance
class CryptoHashRun(xbx.run.Run):
    """Contains data specific to cryptographic hash runs
    
    Attributes of interest:
        msg_len     Message length
    """
    __tablename__ = "crypto_hash_run"

    id = Column(Integer)
    msg_len = Column(Integer)

    __mapper_args__ = {
        'polymorphic_identity':'crypto_hash_run',
    }

    __table_args__ = (
        PrimaryKeyConstraint("id"),
        ForeignKeyConstraint(["id"], ["run.id"]))

    def __init__(self, build_exec,**kwargs):
        super().__init__(build_exec, **kwargs)

    def _assemble_params(self):
        data = bytearray(struct.pack("!I",self.msg_len))
        data += os.urandom(self.msg_len)
        return data 
    
    @classmethod
    def run(cls, build_exec, params=None):
        """Factory method that generates run instances and attaches them to
        buildexec. Call this instead of constructor"""
        _logger.info("Running benchmark on {} with msg length {}".
                     format(build_exec.build, params[0]))
        run = cls(build_exec, msg_len=params[0])
        run._execute()
        return run



class CryptoAeadRun(xbxr.Run):
    """Contains data specific to AEAD runs

    Attributes of interest:

        msg_len         Message Length

        auth_data_len   Authenticated Data Length

        direction       Encryption or Decryption

    """
    __tablename__ = "crypto_aead_run"

    id = Column(Integer)
    msg_len = Column(Integer)
    auth_data_len = Column(Integer)
    direction = Column(Enum("enc", "dec"))

    __mapper_args__ = {
        'polymorphic_identity':'crypto_aead_run',
    }

    __table_args__ = (
        PrimaryKeyConstraint("id"),
        ForeignKeyConstraint(["id"], ["run.id"]))

    ENCRYPT="enc"
    DECRYPT="dec"

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
    
    @classmethod
    def run(cls, build_exec, params=None):
        """Factory method that generates run instances and attaches them to
        buildexec. Call this instead of constructor"""
        _logger.info("Running benchmark on {} with msg length {}".
                     format(build_exec.build, params[0]))
        enc_run = cls(build_exec, params, direction=CryptoAeadRun.ENCRYPT)
        dec_run = cls(build_exec, params, direction=CryptoAeadRun.DECRYPT)
        run._execute()
        return run


OPERATIONS={
    "crypto_aead": (CryptoAeadRun, xbh.TypeCode.AEAD),
    "crypto_hash": (CryptoHashRun, xbh.TypeCode.HASH),
}

