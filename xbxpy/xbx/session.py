import logging
import subprocess
import socket
import datetime

from sqlalchemy.schema import ForeignKeyConstraint, PrimaryKeyConstraint
from sqlalchemy import Column, ForeignKey, Integer, String, Text, Boolean, DateTime
from sqlalchemy.ext.declarative import declared_attr
from sqlalchemy.orm import relationship

from xbx.database import Base

logger = logging.getLogger(__name__)
class SessionMixin: 
    id          = Column(Integer, primary_key=True)
    host        = Column(String)
    timestamp   = Column(DateTime)
    xbx_version = Column(String)

    @declared_attr
    def config_hash(cls): 
        return Column(String, ForeignKey("config.hash"))

    @declared_attr
    def config(cls):
        return relationship("Config", uselist=False)


    def _setup_session(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.host = socket.gethostname()
        self.timestamp = datetime.datetime.now()
        
        try:
            self.xbx_version = subprocess.check_output(
                    ["git", "rev-parse", "HEAD"]
                    ).decode().strip()
        except subprocess.CalledProcessError:
            logger.warn("Could not get git revision of xbx")



