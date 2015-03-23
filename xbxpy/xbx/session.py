import logging
import subprocess
import socket
import datetime

logger = logging.getLogger(__name__)
class Session: 
    def __init__(self, config, database):
        self.config = config
        self.database = database
        self.host = socket.gethostname()
        self.timestamp = datetime.datetime.now()
        
        try:
            self.xbx_version = subprocess.check_output(
                    ["git", "rev-parse", "HEAD"]
                    ).decode().strip()
        except subprocess.CalledProcessError:
            logger.warn("Could not get git revision of xbx")

        self.session_id = None


