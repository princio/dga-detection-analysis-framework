
from dataclasses import dataclass
import json
import os
from pathlib import Path
import shutil
from typing import List
import psycopg2

import sqlalchemy


@dataclass
class ConfigBin:
    dns_parse: Path = Path()
    tcpdump: Path = Path()
    tshark: Path = Path()
    python_lstm: Path = Path()
    python_psl_list: Path = Path()
    psltrie: Path = Path()
    pass

@dataclass
class ConfigPyScript:
    lstm: Path = Path()
    psl_list: Path = Path()
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
    sha256: str = ""
    dga: int = -1
    info: str = ""
    pass

@dataclass
class ConfigPCAP:
    path: Path = Path()
    name: str = ""
    infected: bool = True
    dataset: str = ""
    terminal: str = ""
    day: int = 0
    days: int = 0
    sha256: str = ""
    malware: ConfigMalware = ConfigMalware()
    year: int = 0
    pass

@dataclass
class ConfigWhitelist:
    name: str = ""
    path: str = ""
    year: int = 2000
    dn_col: str = "dn"
    bdn_col: str = "bdn"
    rank_col: str = "rank"
    id: int = -1
    pass

@dataclass
class ConfigWorkdir:
    path: Path = Path()
    dns_parse: Path = Path()
    log: Path = Path()
    psl_list_dir: Path = Path()
    psl_list_path: Path = Path()
    tshark: Path = Path()

    def __init__(self, workdir: Path):
        self.path = workdir

        self.path.mkdir(parents=True, exist_ok=True)

        self.psl_list_dir = workdir.joinpath("psl_list")
        self.psl_list_dir.mkdir(parents=True, exist_ok=True)
        self.psl_list_path = self.psl_list_dir.joinpath("psl_list.csv")

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
    pass

class Config:
    def __init__(self, configjson: Path, workdir: Path):
        self.workdir = ConfigWorkdir(workdir)

        with open(configjson, "r") as fp_conf:
            conf = json.load(fp_conf)

            self.postgre = ConfigPostgre()
            self.postgre.user = conf["postgre"]["user"]
            self.postgre.dbname = conf["postgre"]["dbname"]
            self.postgre.host = conf["postgre"]["host"]
            self.postgre.password = conf["postgre"]["password"]
            self.lstm_batch_size = conf["lstm_batch_size"]
                
            self.bin = ConfigBin()
            self.bin.tcpdump = Path(conf["bin"]["tcpdump"])
            self.bin.tshark = Path(conf["bin"]["tshark"])
            self.bin.python_psl_list = Path(conf["bin"]["python_psl_list"])
            self.bin.python_lstm = Path(conf["bin"]["python_lstm"])
            self.bin.dns_parse = conf["bin"]["dns_parse"]
            self.bin.psltrie = conf["bin"]["psltrie"]

            self.pyscript = ConfigPyScript()
            self.pyscript.psl_list = conf["pyscript"]["psl_list"]
            self.pyscript.lstm = conf["pyscript"]["lstm"]


            self.pcaps: List[Path] = []
            tmp = [Path(pcappath) for pcappath in conf["pcaps"]]
            for pd in tmp:
                if pd.is_dir():
                    [self.pcaps.append(file) for file in pd.glob("*/*.pcap") if file.is_file() ]
                    pass
                else:
                    self.pcaps.append(pd)
                    pass

            self.nndir = conf["nndir"]

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

            self.whitelists: List[ConfigWhitelist] = []
            for whitelist in conf["whitelists"]:
                configwhitelist = ConfigWhitelist()
                configwhitelist.name = whitelist["name"]
                configwhitelist.path = whitelist["path"]
                configwhitelist.year = whitelist["year"]
                configwhitelist.dn_col = whitelist["dn_col"]
                configwhitelist.bdn_col = whitelist["bdn_col"]
                configwhitelist.rank_col = whitelist["rank_col"]
                self.whitelists.append(configwhitelist)
            pass
        pass

    pass
