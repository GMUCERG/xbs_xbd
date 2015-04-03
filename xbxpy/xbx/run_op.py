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
        run._execute(run.__assemble_params())
        return run



class CryptoAeadRun(xbxr.Run):
    """Contains data specific to AEAD runs

    Attributes of interest:

        plaintext_len    Plaintext message Length

        assoc_data_len   Associated Data Length

        direction        CryptoAeadRun.ENCRYPT or CryptoAeadRun.DECRYPT

    """
    ENCRYPT="enc"
    DECRYPT="dec"
    __tablename__ = "crypto_aead_run"

    id = Column(Integer)
    pair_id = Column(Integer)
    msg_len = Column(Integer)
    assoc_data_len = Column(Integer)
    direction = Column(Enum(ENCRYPT, DECRYPT))

    __mapper_args__ = {
        'polymorphic_identity':'crypto_aead_run',
    }

    __table_args__ = (
        PrimaryKeyConstraint("id"),
        ForeignKeyConstraint(["id"], ["run.id"]))


    def __init__(self, build_exec, params=None, **kwargs):
        super().__init__(build_exec, params,  **kwargs)
        self.plaintext_len = params[0]
        self.assoc_data_len = params[1]

    def _gen_enc_params(self):
        macros = self.buildexec.build.implementation.macros

        key_bytes = macros['_KEYBYTES']
        secret_number_bytes = macros['_NSECBYTES']
        public_number_bytes = macros['_NPUBBYTES']
        header = struct.pack(
            "!IIIII",
            self.plaintext_len, 
            self.assoc_data_len,
            secret_number_bytes,
            public_number_bytes,
            key_bytes
        )
        plaintext  = os.urandom(self.plaintext_len)
        assoc_data = os.urandom(self.assoc_data_len)
        secret_num = os.urandom(secret_number_bytes)
        public_num = os.urandom(public_number_bytes)
        key        = os.urandom(key_bytes)

        data = b''.join((header, 
                         plaintext, 
                         assoc_data, 
                         secret_num, 
                         public_num,
                         key))

        return data, plaintext, assoc_data, key, secret_num, public_num

    def _assemble_dec_params(self, ciphertext, assoc_data, public_num, key):
        header = bytearray(
            struct.pack(
                "!IIII",
                len(ciphertext),
                len(assoc_data),
                len(public_num),
                len(key),
            )
        )

        data = b''.join((header,
                         ciphertext, 
                         assoc_data, 
                         public_num, 
                         key))
        
        return data

    
    @classmethod
    def run(cls, build_exec, params=None):
        """Factory method that generates run instances and attaches them to
        buildexec. Call this instead of constructor"""
        _logger.info("Running benchmark on {} with msg length {}".
                     format(build_exec.build, params[0]))
        s = xbxdb.scoped_session()
        enc_run = cls(build_exec, params, direction=CryptoAeadRun.ENCRYPT)
        dec_run = cls(build_exec, params, direction=CryptoAeadRun.DECRYPT)

        (data, plaintext, assoc_data, key, secret_num, public_num) = enc_run._gen_enc_params() 

        # Set identical pair ids so we know encryption/decryption pairs
        enc_run.pair_id = enc_run.id
        dec_run.pair_id = enc_run.id
        enc_run._execute()
        dec_run._execute()
        return enc_run,dec_run


OPERATIONS={
    "crypto_aead": (CryptoAeadRun, xbh.TypeCode.AEAD),
    "crypto_hash": (CryptoHashRun, xbh.TypeCode.HASH),
}

