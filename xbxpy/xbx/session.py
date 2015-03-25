import logging
import subprocess
import socket
import datetime

from sqlalchemy.schema import ForeignKeyConstraint, PrimaryKeyConstraint
from sqlalchemy import Column, ForeignKey, Integer, String, Text, Boolean, Date
from sqlalchemy.ext.declarative import AbstractConcreteBase, declared_attr
from sqlalchemy.orm import relationship

from xbx.database import Base

logger = logging.getLogger(__name__)
class Session(AbstractConcreteBase, Base): 
    id          = Column(Integer)
    config_hash = Column(String)
    host        = Column(String)
    timestamp   = Column(Date)
    xbx_version = Column(String)


    @declared_attr
    def config(cls):
        return relationship("Config", uselist=False)

    __table_args__ = (
        PrimaryKeyConstraint("id"),
        ForeignKeyConstraint(["config_hash"], ["config.hash"]),
    )

    def __init__(self, config, **kwargs):
        super().__init__(**kwargs)
        self.config = config
        self.host = socket.gethostname()
        self.timestamp = datetime.datetime.now()
        
        try:
            self.xbx_version = subprocess.check_output(
                    ["git", "rev-parse", "HEAD"]
                    ).decode().strip()
        except subprocess.CalledProcessError:
            logger.warn("Could not get git revision of xbx")



