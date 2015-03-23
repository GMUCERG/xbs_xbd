import logging
import subprocess

logger = logging.getLogger(__name__)
class Session: 
    def __init__(self, config, database):
        self.config = config
        self.database = database
        
        try:
            self.xbx_version = subprocess.check_output(
                    ["git", "rev-parse", "HEAD"]
                    ).decode().strip()
        except subprocess.CalledProcessError:
            logger.warn("Could not get git revision of xbx")

