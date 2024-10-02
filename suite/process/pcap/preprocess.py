from dataclasses import dataclass
from pathlib import Path
import re

import numpy as np

from utils import Subprocess
import pandas as pd

@dataclass
class PCAPBinaries:
    dns_parse: Path = Path()
    tcpdump: Path = Path()
    tshark: Path = Path()
    pass

class PCAPPreProcess:
    def __init__(self, binaries: PCAPBinaries, pcapfile: Path, outdir: Path) -> None:
        
        self.pcapfile = pcapfile
        self.outdir = outdir.joinpath(self.pcapfile.name)

        if not self.outdir.exists():
            self.outdir.mkdir(parents=True, exist_ok=True)
            pass

        self.tshark = Subprocess('tshark', binaries.tshark, self.outdir)
        self.tcpdump = Subprocess('tcpdump', binaries.tcpdump, self.outdir)
        self.dns_parse = Subprocess('dns_parse', binaries.dns_parse, self.outdir)
        pass

    def run_tcpdump(self):
        self.tcpdump.launch(
            self.pcapfile,
            [
                "-r", Subprocess.INPUT_FLAG,
                "-w", Subprocess.OUTPUT_FLAG,
                "port", "53"
            ])
        pass

    def run_tshark(self):
        self.tshark.launch(
            self.tcpdump.output, [
                "-r", Subprocess.INPUT_FLAG,
                "-Y", "dns && !_ws.malformed && !icmp",
                "-w", Subprocess.OUTPUT_FLAG
            ])
        pass

    def run_dns_parse(self):
        self.dns_parse.launch(
            self.tshark.output, [
                "-o", Subprocess.OUTPUT_FLAG,
                Subprocess.INPUT_FLAG
            ])
        pass

    def run(self):
        self.run_tcpdump()
        self.run_tshark()
        self.run_dns_parse()

        output = self.dns_parse.output

        # with open(output, 'r') as fp:
        #     reg = re.compile(r"(?<=,)\"|\"(?=,)")
        #     lines = [ reg.sub('|', line) for line in fp.readlines()]
        #     pass

        # with open(output, 'w') as fp:
        #     fp.writelines(lines)
        #     pass

        df = pd.read_csv(output, index_col=False)

        df['server'] = df['server'].replace([np.nan], [None])
        df['answer'] = df['answer'].replace([np.nan], [None])

        df.index.set_names("fn", inplace=True)
        df = df.reset_index()
        
        return df
    pass