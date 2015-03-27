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

from sqlalchemy.orm import scoped_session as ss, sessionmaker as sm
scoped_session = ss(sm())
def init(data_path):

    global scoped_session
    engine = None
    if not os.path.isfile(data_path):
        _logger.info("Database doesn't exist, initializing")
        # Import these so ORM builds tables
        import xbx.config
        import xbx.build
        import xbx.run
        import xbx.run_op
        engine = create_engine('sqlite:///'+ data_path)
        Base.metadata.create_all(engine)
    else:
        engine = create_engine('sqlite:///'+ data_path)

    scoped_session.configure(bind=engine)



# Enforce foreign keys
from sqlalchemy.engine import Engine
from sqlalchemy import event

@event.listens_for(Engine, "connect")
def set_sqlite_pragma(dbapi_connection, connection_record):
    cursor = dbapi_connection.cursor()
    cursor.execute("PRAGMA foreign_keys=ON")
    cursor.close()




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
    """Prevents duplicate errors in DB
    
    Decorator on data object constructors to ensure new objects aren't created
    if already exists in DB 

    See https://bitbucket.org/zzzeek/sqlalchemy/wiki/UsageRecipes/UniqueObject

    """
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





# SQLAlchemy typedecorator recipe
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






