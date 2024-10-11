from pathlib import Path

import numpy as np

from config import Config
from utils import subprocess_launch

import pandas as pd

class PCAPPreProcess:
    def __init__(self, config: Config, pcapfile: Path) -> None:
        self.config: Config = config
        
        self.pcapfile = pcapfile
        self.pcapdir = self.config.workdir.path.joinpath(self.pcapfile.name)

        if not self.pcapdir.exists():
            self.pcapdir.mkdir(parents=True, exist_ok=True)
            pass

        self.tcpdump_path: Path = self.pcapdir.joinpath(self.pcapfile.with_suffix(".tcpdump.pcap").name)
        self.tshark_path: Path = self.pcapdir.joinpath(self.pcapfile.with_suffix(".dns.pcap").name)
        self.dns_parse_path: Path = self.pcapdir.joinpath(self.pcapfile.with_suffix(".csv").name)

        self.qr = None

        pass

    def run_psl_list(self):
        subprocess_launch(self.config,
                          self.config.workdir.psl_list_path,
                          "psl_list", [
                              self.config.bin.python_psl_list,
                              self.config.pyscript.psl_list,
                              "-w", self.config.workdir.psl_list_dir,
                              self.config.workdir.psl_list_path
                        ])
        pass

    def run_tshark(self):
        subprocess_launch(self.config,
                          self.tshark_path,
                          "tshark", [
                              self.config.bin.tshark,
                              "-r", self.tcpdump_path,
                              "-Y", "dns && !_ws.malformed && !icmp",
                              "-w", self.tshark_path
                        ])
        
        pass

    def run_tcpdump(self):
        subprocess_launch(self.config,
                          self.tcpdump_path,
                          "tcpdump", [
                              self.config.bin.tcpdump,
                              "-r", self.pcapfile,
                              "-w", self.tcpdump_path,
                              "port", "53"
                        ])
        
        pass

    def run_dns_parse(self):
        subprocess_launch(self.config,
                          self.dns_parse_path,
                          "dns_parse", [
                              self.config.bin.dns_parse,
                              "-o", self.dns_parse_path,
                              self.tshark_path
                        ])
        pass

    def run(self):
        self.run_psl_list()
        self.run_tcpdump()
        self.run_tshark()
        self.run_dns_parse()

        df = pd.read_csv(self.dns_parse_path, index_col=False)

        df.server.replace([np.nan], [None], inplace=True)
        df.answer.replace([np.nan], [None], inplace=True)

        df.index.set_names("fn", inplace=True)
        df = df.reset_index()
        
        return df
    pass