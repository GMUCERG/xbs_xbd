import logging
import struct
import sys
import os

from sqlalchemy.schema import ForeignKeyConstraint, PrimaryKeyConstraint
from sqlalchemy import Column, ForeignKey, Integer, String, Text, Boolean, Date, Enum
from sqlalchemy.orm import relationship
from sqlalchemy.ext.declarative import declarative_base

import xbx.database as xbxdb
import xbx.run as xbxr
#from xbx.run import Run
import xbx.session
import xbh

_logger = logging.getLogger(__name__)

class CryptoHashRun(xbxr.Run):
    """Contains data specific to cryptographic hash runs

    Call run() to instantiate and run, do not use constructor

    Attributes of interest:
        msg_len     Message length
    """
    __tablename__ = "crypto_hash_run"

    id = Column(Integer, nullable=False)
    msg_len = Column(Integer)

    __mapper_args__ = {
        'polymorphic_identity':'crypto_hash_run',
    }

    __table_args__ = (
        PrimaryKeyConstraint("id"),
        ForeignKeyConstraint(["id"], ["run.id"]))

    def _assemble_params(self):
        data = os.urandom(self.msg_len)
        return data

    @classmethod
    def run(cls, build_exec, params=None):
        """Factory method that generates run instances and attaches them to
        build_exec. Call this instead of constructor"""
        _logger.info("Running benchmark on {} with msg length {}".
                     format(build_exec.build.buildid, params[0]))
        run = cls(build_exec, msg_len=params[0])
        run._execute(run._assemble_params())
        run._calculate_power()
        return run


class CryptoAeadRun(xbxr.Run):
    """Contains data specific to AEAD runs

    Call run() to instantiate and run, do not use constructor

    Attributes of interest:

        plaintext_len    Plaintext message Length

        assoc_data_len   Associated Data Length

        mode             CryptoAeadRun.ENCRYPT or CryptoAeadRun.DECRYPT or
                         CryptoAeadRun.FORGED_DEC

        data_id      ID number to identify crypt,decrypt,forge_dec runs as
                         working with the same data

    """
    ENCRYPT="enc"
    DECRYPT="dec"
    FORGED_DEC ="frg"
    __tablename__ = "crypto_aead_run"

    id = Column(Integer, nullable=False)
    data_id = Column(Integer)
    plaintext_len = Column(Integer)
    assoc_data_len = Column(Integer)
    mode = Column(Enum(ENCRYPT, DECRYPT, FORGED_DEC))

    __mapper_args__ = {
        'polymorphic_identity':'crypto_aead_run',
    }

    __table_args__ = (
        PrimaryKeyConstraint("id"),
        ForeignKeyConstraint(["id"], ["run.id"]))

    def _gen_enc_params(self):
        macros = self.build_exec.build.implementation.macros

        key_bytes = macros['_KEYBYTES']
        secret_number_bytes = macros['_NSECBYTES']
        public_number_bytes = macros['_NPUBBYTES']
        header = struct.pack(
            "!IIIIII",
            0,          # Encrypt
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

    @staticmethod
    def _assemble_dec_params(ciphertext, assoc_data, public_num, key):
        header = bytearray(
            struct.pack(
                "!IIIII",
                1,      # Decrypt
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
        """Factory method to create and execute run instances.

        Call this instead of constructor
        """
        _logger.info("Running benchmark on {} with plaintext length {} and "
                     "associated data length {}".
                     format(build_exec.build.buildid, params[0], params[1]))

        primitive = build_exec.build.primitive;


        enc_run = cls(build_exec, plaintext_len=params[0],
                      assoc_data_len=params[1], mode=CryptoAeadRun.ENCRYPT)
        dec_run = cls(build_exec, plaintext_len=params[0],
                      assoc_data_len=params[1], mode=CryptoAeadRun.DECRYPT)
        dec_forged_run = cls(build_exec, plaintext_len=params[0],
                      assoc_data_len=params[1], mode=CryptoAeadRun.FORGED_DEC)

        # Set identical pair ids so we know encryption/decryption pairs
        s = xbxdb.scoped_session()
        s.add(enc_run)
        s.add(dec_run)
        s.add(dec_forged_run)
        s.flush()  # Need to add and flush to get id
        enc_run.data_id = enc_run.id
        dec_run.data_id = enc_run.id
        dec_forged_run.data_id = enc_run.id

        # Generate data
        (enc_data, plaintext, assoc_data,
         key, secret_num, public_num) = enc_run._gen_enc_params()
        
        retval, ciphertext = enc_run._execute(enc_data)
        enc_run._calculate_power()

        if retval != 0:
            raise xbxr.XbdResultFailError(
                "XBD execute returned {}".format(retval))

        dec_data = CryptoAeadRun._assemble_dec_params(ciphertext, assoc_data,
                                                  public_num, key)

        retval, dec_output = dec_run._execute(dec_data)
        dec_run._calculate_power()

        # Unpack lengths
        # We put secret num first since it is fixed length
        fmt = "!II"
        secnum_len, plaintext_len = struct.unpack_from(fmt, dec_output)

        # Unpack data
        (decrypted_secret_num,
         decrypted_plaintext) = struct.unpack(
             "{}s{}s".format(secnum_len, plaintext_len),
             dec_output[struct.calcsize(fmt):]
         )

        if ((decrypted_plaintext != plaintext or
             decrypted_secret_num != secret_num) and
                primitive.name != '0cipher'):
            raise xbxr.XbdResultFailError(
                "Decrypted plaintext does not match CipherText")

        forged_ciphertext = bytearray(ciphertext)

        #Tamper with last byte of ciphertext
        if len(ciphertext):
            forged_ciphertext[-1] = (forged_ciphertext[-1] + 1) % 256

        forged_data = CryptoAeadRun._assemble_dec_params(forged_ciphertext,
                                                         assoc_data, public_num,
                                                         key)

        retval,_ = dec_forged_run._execute(forged_data)
        #dec_forged_run = _calculate_power()
        dec_forged_run._calculate_power()
        
        if retval != -1 and primitive.name != '0cipher':
            raise xbxr.XbdResultFailError(
                "Message forgery not detected")

        return enc_run, dec_run, dec_forged_run


OPERATIONS={
    "crypto_aead": (CryptoAeadRun, xbh.TypeCode.AEAD),
    "crypto_hash": (CryptoHashRun, xbh.TypeCode.HASH),
}

