#! /usr/bin/python3
import configparser
import os
import re


class Config:
    """Configuration for current run"""

    all_platforms_path = ""
    algopack_path = ""
    embedded_path = ""
    work_path = ""

    platform_path = ""
    platform_templ_path = ""
    platform = ""
    clock_hz = 0

    operation = ""
    algorithms = {}



    def __init__(self, filename):
        config = configparser.ConfigParser()
        config.read(filename)

        ## Basic path configuration
        self.all_platforms_path = config['paths']['platforms']
        self.algopack_path = config['paths']['algopacks']
        self.embedded_path = config['paths']['embedded']
        self.work_path     = config['paths']['work']


        ## Platform configuration
        self.platform = config['hardware']['platform']
        self.platform_path = os.path.join(self.all_platforms_path, self.platform)
        config.read(os.path.join(self.platform_path, "settings.ini"))
        if config.has_option('platform_settings', 'templatePlatform'):
            self.platform_templ_path = os.path.join(
                    self.all_platforms_path, config['platform_settings']['templatePlatform'])
        self.clock_hz = config['platform_settings']['cyclespersecond']
        self.pagesize = config['platform_settings']['pagesize']



        ## Get algorithms and implementations
        self.operation = config['algorithm']['operation']
        algorithms = config['algorithm']['algorithms'].split("\n")

        # Get operation path
        op_path = os.path.join(self.algopack_path, "crypto_"+self.operation)

        # Get list of algorithm directories
        algodirs = [d for d in os.listdir(op_path) if os.path.isdir(os.path.join(op_path, d))]

        # Get algo directories intersecting w/ algorithm list
        for i in set(algorithms) & set(algodirs):
            class Algorithm:
                pass
            # Insert algorithm as key into self.algorithms
            # path and implementations as value
            self.algorithms[i] = Algorithm()
            self.algorithms[i].path = os.path.join(op_path,i)
            self.algorithms[i].impls = {}

        blacklist = []
        whitelist = []

        if config.has_option('implementation', 'blacklist') and config['implementation']['blacklist']:
            blacklist = config['implementation']['blacklist'].split("\n")

        if config.has_option('implementation', 'whitelist') and config['implementation']['whitelist']:
            whitelist = config['implementation']['whitelist'].split("\n")

        # Get whitelist  ^blacklist
        for algorithm, item in self.algorithms.items():
            alldirs = [d for d in os.listdir(item.path) if
                    os.path.isdir(os.path.join(item.path, d))]

            keptdirs = set(alldirs)

            for i in alldirs:
                for j in blacklist:
                    if re.match(j,i):
                        keptdirs.remove(i)

            if whitelist:
                keptdirs &= set(whitelist);

            for i in keptdirs:
                item.impls[i] = os.path.join(item.path,i)

