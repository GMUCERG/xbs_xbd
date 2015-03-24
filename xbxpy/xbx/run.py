import logging
import sys

import xbx.data
import xbx.session
import xbh

logger = logging.getLogger(__name__)

class Run:
    def __init__(self, config):
        pass



class RunSession(xbx.session.Session):
    """Manages runs specified in config and that are defined in the last
    BuildSession

    Pass in database object into constructor to save session and populate id
    """

    def __init__(self, config, database=None):
        super().__init__(config, database)
        if self.database:
            self.buildsession = database.get_newest_buildsession()
            self.session_id = self.database.save_runsession(self)

        self.xbh = None


    def init_xbh(self):
        c = self.config
        try:
            self.xbh = xbh.Xbh(
                    c.xbh_addr, c.xbh_port, 
                    c.platform.pagesize,
                    c.platform.clock_hz)
        except xbh.XbhError as e:
            logger.critical(str(e))
            sys.exit(1)

    def drift_measurements(self):
        for i in range(10):
            (abs_error, 
             rel_error, 
             cycles, 
             measured_cycles) = self.xbh.timing_error_calc()  

            logger.debug(("Abs Error: {}, Rel Error: {}, "
                "Cycles: {}, Measured Cycles: {}").format(
                        abs_error, rel_error, cycles, measured_cycles))

            pass

            






    def runall(self):
        pass






        


