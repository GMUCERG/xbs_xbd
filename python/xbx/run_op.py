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

class CryptoKemRun(xbxr.Run):
    """Contains data specific to cryptographic kem runs

    Call run() to instantiate and run, do not use constructor

    Attributes of interest:
        mode        KEYPAIR, ENCAPS, or DECAPS

        data_id      ID number to identify crypt,decrypt runs as
                         working with the same data
    """
    KEYPAIR="keypair"
    ENCAPS="enc"
    DECAPS="dec"
    FRG="frg"

    __tablename__ = "crypto_kem_run"

    id = Column(Integer, nullable=False)
    data_id = Column(Integer)
    mode = Column(Enum(KEYPAIR, ENCAPS, DECAPS, FRG))
    secret_key_len = Column(Integer)
    public_key_len = Column(Integer)

    __mapper_args__ = {
        'polymorphic_identity':'crypto_kem_run',
    }

    __table_args__ = (
        PrimaryKeyConstraint("id"),
        ForeignKeyConstraint(["id"], ["run.id"]))


    def _gen_keypair_params(self):
        header = struct.pack(
            "!I",
            0            # KEYPAIR
        )
        return header


    def _assemble_enc_params(self, pubkeyptr):
        #header = struct.pack(
        #    "!II",
        #    1,          # ENCAPSULATE
        #    self.public_key_len
        #)
        #return b''.join((header, pubkey))
        header = struct.pack(
            "!II",
            1,          # ENCAPSULATE
            pubkeyptr
        )
        return header


    def _assemble_dec_params(self, ciphertext, seckeyptr):
        macros = self.build_exec.build.implementation.macros

        ciphertext_bytes = macros['_CIPHERTEXTBYTES']
        #header = struct.pack(
        #    "!III",
        #    2,            # DECAPSULATE
        #    ciphertext_bytes,
        #    self.secret_key_len
        #)
        #return b''.join((header, ciphertext, seckey))
        header = struct.pack(
            "!III",
            2,            # DECAPSULATE
            seckeyptr,
            ciphertext_bytes
        )
        return b''.join((header, ciphertext))


    @classmethod
    def run(cls, build_exec, params=None):
        """Factory method to create and execute run instances.

        Call this instead of constructor
        """
        _logger.info("Running benchmark on {}".format(build_exec.build.buildid))

        xbh = build_exec.run_session.xbh
        macros = build_exec.build.implementation.macros

        # Set identical pair ids so we know encryption/decryption pairs
        s = xbxdb.scoped_session()

        #keypair_run = cls(build_exec, mode=CryptoKemRun.KEYPAIR)
        #s.add(keypair_run)
        #s.flush()  # Need to add and flush to get id
        #keypair_run.data_id = keypair_run.id
        #keypair_run.secret_key_len = macros['_SECRETKEYBYTES']
        #keypair_run.public_key_len = macros['_PUBLICKEYBYTES']

        ## Generate data
        #keypair_data = keypair_run._gen_keypair_params()

        #retval, keypair_output = keypair_run._execute(keypair_data)
        #keypair_run._calculate_power()

        #if retval != 0:
        #    raise xbxr.XbdResultFailError(
        #        "XBD execute returned {}".format(retval))

        ## Unpack keypair lengths
        #fmt = "!II"
        #pubkey_len, seckey_len = struct.unpack_from(fmt, keypair_output)
        #_logger.debug("seckey_len={}".format(seckey_len))
        #_logger.debug("pubkey_len={}".format(pubkey_len))

        ## Unpack keypair
        #pubkey, seckey = struct.unpack(
        #    "{}s{}s".format(pubkey_len, seckey_len),
        #    keypair_output[struct.calcsize(fmt):]
        #)

        ################################################################################
        # START of key files
        #
        # This is a temporary solution until the i2c comm can be fixed. XBH seems to
        # crash whenever ~3000 bytes are received from XBD. This limited the number of
        # algorithms I could test. Because of time constraints, I did not dig too deep
        # for a better solution and implemented this.
        ################################################################################
        import base64

        keydir = "xbdkeys"
        op = "crypto_kem"
        algo = build_exec.build.implementation.primitive.name
        impl = build_exec.build.implementation.name
        pubkeypath = os.path.join(keydir, op, algo, impl, str(params[0]) + ".pub")
        seckeypath = os.path.join(keydir, op, algo, impl, str(params[0]) + ".sec")
        if not os.path.isfile(pubkeypath) or not os.path.isfile(seckeypath):
            print(pubkeypath, "or", seckeypath, "not found")
            sys.exit(1) # no better solution for now

        # Load the keys
        pubkeystr = b''
        with open(pubkeypath, 'rb') as f:
            pubkeystr = f.read()
        seckeystr = b''
        with open(seckeypath, 'rb') as f:
            seckeystr = f.read()
        pubkey = base64.decodestring(pubkeystr)
        seckey = base64.decodestring(seckeystr)
        ################################################################################
        # END of key files
        ################################################################################

        enc_run = cls(build_exec, mode=CryptoKemRun.ENCAPS)
        s.add(enc_run)
        s.flush()  # Need to add and flush to get id
        enc_run.data_id = enc_run.id
        enc_run.secret_key_len = macros['_SECRETKEYBYTES']
        enc_run.public_key_len = macros['_PUBLICKEYBYTES']

        pubkeyptr = xbh.upload_static_param(pubkey)
        enc_data = enc_run._assemble_enc_params(pubkeyptr)
        
        retval, enc_output = enc_run._execute(enc_data)
        enc_run._calculate_power()

        if retval != 0:
            raise xbxr.XbdResultFailError(
                "XBD execute returned {}".format(retval))

        # Unpack ciphertext and sessionkey lengths
        fmt = "!II"
        clen, klen = struct.unpack_from(fmt, enc_output)
        _logger.debug("clen={}".format(clen))
        _logger.debug("klen={}".format(klen))

        # Unpack ciphertext and sessionkey
        c, k = struct.unpack(
            "{}s{}s".format(clen, klen),
            enc_output[struct.calcsize(fmt):]
        )

        # Retrieved session key!

        dec_run = cls(build_exec, mode=CryptoKemRun.DECAPS)
        s.add(dec_run)
        s.flush()  # Need to add and flush to get id
        dec_run.data_id = enc_run.id
        dec_run.secret_key_len = macros['_SECRETKEYBYTES']
        dec_run.public_key_len = macros['_PUBLICKEYBYTES']

        seckeyptr = xbh.upload_static_param(seckey)
        dec_data = dec_run._assemble_dec_params(c, seckeyptr)

        retval, k2 = dec_run._execute(dec_data)
        dec_run._calculate_power()

        if retval != 0:
            raise xbxr.XbdResultFailError(
                "XBD execute returned {}".format(retval))

        if k != k2:
            raise xbxr.XbdResultFailError(
                "Decrypted session key does not match encapsulated key")

        # Tamper with the ciphertext
        c2 = bytearray(c)
        if len(c2):
            c2[0] = (c2[0] + 1) % 256
            c2[-1] = (c2[-1] + 1) % 256

        frg_run = cls(build_exec, mode=CryptoKemRun.FRG)
        s.add(frg_run)
        s.flush()  # Need to add and flush to get id
        frg_run.data_id = enc_run.id
        frg_run.secret_key_len = macros['_SECRETKEYBYTES']
        frg_run.public_key_len = macros['_PUBLICKEYBYTES']

        seckeyptr = xbh.upload_static_param(seckey)
        frg_data = frg_run._assemble_dec_params(c2, seckeyptr)

        retval, k3 = frg_run._execute(frg_data)
        frg_run._calculate_power()

        if retval == 0 and k == k3:
            kstr = "".join(["{:x}".format(cbyte) for cbyte in k])
            k3str = "".join(["{:x}".format(cbyte) for cbyte in k3])
            raise xbxr.XbdResultFailError(
                "Decrypted session key matches forgery k={} :: k2={}".format(kstr,k3str))

        return enc_run, dec_run, frg_run


class CryptoSignRun(xbxr.Run):
    """Contains data specific to cryptographic signature runs

    Call run() to instantiate and run, do not use constructor

    Attributes of interest:
        msg_len     Message length

        mode        KEYPAIR, SIGN, OPEN, FRG

        data_id     ID number to identify crypt,decrypt,forge_dec runs as
                         working with the same data
    """
    KEYPAIR="keypair"
    SIGN="sign"
    OPEN="open"
    FRG="frg"

    __tablename__ = "crypto_sign_run"

    id = Column(Integer, nullable=False)
    data_id = Column(Integer)
    msg_len = Column(Integer)
    mode = Column(Enum(KEYPAIR, SIGN, OPEN, FRG))
    secret_key_len = Column(Integer)
    public_key_len = Column(Integer)

    __mapper_args__ = {
        'polymorphic_identity':'crypto_sign_run',
    }

    __table_args__ = (
        PrimaryKeyConstraint("id"),
        ForeignKeyConstraint(["id"], ["run.id"]))


    def _gen_keypair_params(self):
        header = struct.pack(
            "!I",
            0            # KEYPAIR
        )
        return header


    def _assemble_sign_params(self, seckeyptr):
        header = struct.pack(
            "!III",
            1,          # SIGN
            seckeyptr,
            self.msg_len
        )
        m = os.urandom(self.msg_len)
        return b''.join((header, m)), m


    def _assemble_open_params(self, sm, pubkeyptr):
        macros = self.build_exec.build.implementation.macros

        smlen = macros['_BYTES']
        header = struct.pack(
            "!III",
            2,            # OPEN
            pubkeyptr,
            smlen
        )
        return b''.join((header, sm))


    @classmethod
    def run(cls, build_exec, params=None):
        """Factory method to create and execute run instances.

        Call this instead of constructor
        """
        _logger.info("Running benchmark on {} with message length {}".
                format(build_exec.build.buildid, params[0]))

        xbh = build_exec.run_session.xbh
        macros = build_exec.build.implementation.macros

        # Set identical pair ids so we know encryption/decryption pairs
        s = xbxdb.scoped_session()

        #keypair_run = cls(build_exec, msg_len=params[0],
        #         mode=CryptoSignRun.KEYPAIR)
        #s.add(keypair_run)
        #s.flush()  # Need to add and flush to get id
        #keypair_run.data_id = keypair_run.id
        #keypair_run.secret_key_run = macros['_SECRETKEYBYTES']
        #keypair_run.public_key_run = macros['_PUBLICKEYBYTES']

        ## Generate data
        #keypair_data = keypair_run._gen_keypair_params()

        #retval, keypair_output = keypair_run._execute(keypair_data)
        #keypair_run._calculate_power()

        #if retval != 0:
        #    raise xbxr.XbdResultFailError(
        #        "XBD execute returned {}".format(retval))

        ## Unpack keypair lengths
        #fmt = "!II"
        #pubkey_len, seckey_len = struct.unpack_from(fmt, keypair_output)

        ## Unpack keypair
        #pubkey, seckey = struct.unpack(
        #    "{}s{}s".format(pubkey_len, seckey_len),
        #    keypair_output[struct.calcsize(fmt):]
        #)

        ################################################################################
        # START of key files
        #
        # This is a temporary solution until the i2c comm can be fixed. XBH seems to
        # crash whenever ~3000 bytes are received from XBD. This limited the number of
        # algorithms I could test. Because of time constraints, I did not dig too deep
        # for a better solution and implemented this.
        ################################################################################
        import base64

        keydir = "xbdkeys"
        op = "crypto_sign"
        algo = build_exec.build.implementation.primitive.name
        impl = build_exec.build.implementation.name
        pubkeypath = os.path.join(keydir, op, algo, impl, str(params[1]) + ".pub")
        seckeypath = os.path.join(keydir, op, algo, impl, str(params[1]) + ".sec")
        if not os.path.isfile(pubkeypath) or not os.path.isfile(seckeypath):
            print(pubkeypath, "or", seckeypath, "not found")
            sys.exit(1) # no better solution for now

        # Load the keys
        pubkeystr = b''
        with open(pubkeypath, 'rb') as f:
            pubkeystr = f.read()
        seckeystr = b''
        with open(seckeypath, 'rb') as f:
            seckeystr = f.read()
        pubkey = base64.decodestring(pubkeystr)
        seckey = base64.decodestring(seckeystr)
        ################################################################################
        # END of key files
        ################################################################################

        sign_run = cls(build_exec, msg_len=params[0],
                mode=CryptoSignRun.SIGN)
        s.add(sign_run)
        s.flush()  # Need to add and flush to get id
        sign_run.data_id = sign_run.id
        sign_run.secret_key_len = macros['_SECRETKEYBYTES']
        sign_run.public_key_len = macros['_PUBLICKEYBYTES']

        seckeyptr = xbh.upload_static_param(seckey)
        sign_data, m = sign_run._assemble_sign_params(seckeyptr)

        # Generated message!
        
        retval, sm = sign_run._execute(sign_data)
        sign_run._calculate_power()

        if retval != 0:
            raise xbxr.XbdResultFailError(
                "XBD execute returned {}".format(retval))

        # Retrieved signature!

        open_run = cls(build_exec, msg_len=params[0],
                mode=CryptoSignRun.OPEN)
        s.add(open_run)
        s.flush()  # Need to add and flush to get id
        open_run.data_id = sign_run.id
        open_run.secret_key_len = macros['_SECRETKEYBYTES']
        open_run.public_key_len = macros['_PUBLICKEYBYTES']

        pubkeyptr = xbh.upload_static_param(pubkey)
        open_data = open_run._assemble_open_params(sm, pubkeyptr)

        retval, m2 = open_run._execute(open_data)
        open_run._calculate_power()

        if retval != 0:
            raise xbxr.XbdResultFailError(
                "XBD execute returned {}".format(retval))

        if m != m2:
            raise xbxr.XbdResultFailError(
                "Returned message does not match signed message")

        # Tamper with the signature (sm format: SIGNATURE+MESSAGE)
        sm_frg = bytearray(sm)
        if len(sm_frg):
            sm_frg[0] = (sm_frg[0] + 1) % 256

        open_forged_run = cls(build_exec, msg_len=params[0],
                mode=CryptoSignRun.OPEN)
        s.add(open_forged_run)
        s.flush()  # Need to add and flush to get id
        open_forged_run.data_id = sign_run.id
        open_forged_run.secret_key_len = macros['_SECRETKEYBYTES']
        open_forged_run.public_key_len = macros['_PUBLICKEYBYTES']

        pubkeyptr = xbh.upload_static_param(pubkey)
        forged_data = open_forged_run._assemble_open_params(sm_frg, pubkeyptr)

        retval,_ = open_forged_run._execute(forged_data)
        open_forged_run._calculate_power()
        
        if retval == 0:
            raise xbxr.XbdResultFailError(
                "Message forgery not detected")

        return sign_run, open_run, open_forged_run


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
        dec_forged_run._calculate_power()
        
        if retval != -1 and primitive.name != '0cipher':
            raise xbxr.XbdResultFailError(
                "Message forgery not detected")

        return enc_run, dec_run, dec_forged_run


OPERATIONS={
    "crypto_aead": (CryptoAeadRun, xbh.TypeCode.AEAD),
    "crypto_hash": (CryptoHashRun, xbh.TypeCode.HASH),
    "crypto_kem": (CryptoKemRun, xbh.TypeCode.KEM),
    "crypto_sign": (CryptoSignRun, xbh.TypeCode.SIGN),
}

