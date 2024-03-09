
from dataclasses import dataclass
import json
import os
from pathlib import Path
import shutil
import subprocess
import psycopg2

import sqlalchemy


@dataclass
class ConfigBin:
    dns_parse: Path = Path()
    tshark: Path = Path()
    python: Path = Path()
    python_lstm: Path = Path()
    python_pslregex2: Path = Path()
    pass

@dataclass
class ConfigPyScript:
    lstm: Path = Path()
    pslregex2: Path = Path()
    pass

@dataclass
class ConfigPostgre:
    host: str = ""
    dbname: str = ""
    user: str = ""
    password: str = ""
    pass

@dataclass
class ConfigMalware:
    name: str = ""
    family: str = ""
    year: int = 2000
    md5: str = ""
    sha256: str = ""
    binary: str = ""
    dga: int = -1
    pass

@dataclass
class ConfigPCAP:
    name: str = ""
    infected: bool = True
    dataset: str = ""
    terminal: str = ""
    day: int = 0
    days: int = 0
    sha256: str = ""
    malware: ConfigMalware = ConfigMalware()
    pass

@dataclass
class ConfigJSON:
    config: Path = Path()
    pcap: Path = Path()
    pass

@dataclass
class ConfigWorkdir:
    path: Path = Path()
    dns_parse: Path = Path()
    log: Path = Path()
    pslregex2: Path = Path()
    tshark: Path = Path()

    def __init__(self, workdir: Path):
        self.path = workdir

        self.path.mkdir(parents=True, exist_ok=True)

        self.log = workdir.joinpath("log")
        if not self.log.exists():
            print(f"[info]: making self.log directory: {self.log}")
            self.log.mkdir(parents=True, exist_ok=True)
            pass
        else:
            shutil.rmtree(self.log, ignore_errors=True)
            os.mkdir(self.log)
            print(f"[info]: log directory cleaned.")
            pass
        pass 

        if not self.pslregex2.exists():
            print(f"[info]: making pslregex2 directory: {self.pslregex2}")
            os.mkdir(self.pslregex2)
            pass
        pass
    pass

class Config:
    def __init__(self, configjson: Path, workdir: Path):
        self.workdir = ConfigWorkdir(workdir)

        self.json = ConfigJSON(configjson)
        self.json.config = configjson

        with open(configjson, 'r') as fp_conf:
            conf = json.load(fp_conf)

            self.postgre = ConfigPostgre()
            self.postgre.user = conf['postgre']['user']
            self.postgre.dbname = conf['postgre']['dbname']
            self.postgre.host = conf['postgre']['host']
            self.postgre.password = conf['postgre']['password']
                
            self.bin = ConfigBin()
            self.bin.tshark = Path(conf['bin']['tshark'])
            self.bin.python = Path(conf['bin']['python'])
            self.bin.python_pslregex2 = Path(conf['bin']['python_pslregex2'])
            self.bin.python_lstm = Path(conf['bin']['python_lstm'])
            self.bin.dns_parse = conf['bin']['dns_parse']

            self.pyscript = ConfigPyScript()
            self.pyscript.pslregex2 = conf['pyscript']['pslregex2']
            self.pyscript.lstm = conf['pyscript']['lstm']

            self.json.pcap = conf["json"]["pcap"]
            self.pcap = ConfigPCAP()
            with open(self.json.pcap, 'r') as fp_pcap:
                pcap = json.load(fp_pcap)
                self.pcap.name = pcap['name']
                self.pcap.infected = pcap['infected']
                self.pcap.dataset = pcap['dataset']
                self.pcap.terminal = pcap['terminal']
                self.pcap.day = pcap['day']
                self.pcap.days = pcap['days']
                self.pcap.sha256 = pcap['sha256']
                if self.pcap.infected:
                    self.pcap.malware.name = pcap['malware']['name']
                    self.pcap.malware.year = pcap['malware']['year']
                    self.pcap.malware.md5 = pcap['malware']['md5']
                    self.pcap.malware.sha256 = pcap['malware']['sha256']
                    self.pcap.malware.binary = pcap['malware']['binary']
                    self.pcap.malware.dga = pcap['malware']['dga']
                pass

            self.nndir = conf['nndir']

            url = sqlalchemy.URL.create(
                drivername="postgresql",
                host=self.postgre.host,
                username=self.postgre.user,
                database=self.postgre.dbname,
                password=self.postgre.password
            )
            self.engine: sqlalchemy.Engine = sqlalchemy.create_engine(url)
            self.sqlalchemyconnection: sqlalchemy.Connection = self.engine.connect()
            self.psyconn = psycopg2.connect("host=%s dbname=%s user=%s password=%s" % (self.postgre.host, self.postgre.dbname, self.postgre.user, self.postgre.password))
        pass

    pass
