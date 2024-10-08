from dataclasses import dataclass
from logging import Logger
import logging
from pathlib import Path
from ..subprocess import Subprocess

@dataclass
class PCAPBinaries:
    dns_parse: Path = Path()
    tcpdump: Path = Path()
    tshark: Path = Path()
    pass

class DNSParseService:
    def __init__(self, binary: Path, workdir: Path) -> None:
        self.name = 'dns_parse'
        self.binary = binary
        self.workdir = Path(workdir).joinpath('dns_parse')

        if not self.workdir.exists():
            self.workdir.mkdir(exist_ok=True)
            pass
        pass

        return 

    def run(self, pcapfile: Path):
        workdir = self.workdir.joinpath(pcapfile.name)

        subproc = Subprocess(logging.getLogger(), self.name, self.binary, workdir)
        subproc.launch(
            pcapfile, [
                "-o", Subprocess.OUTPUT_FLAG,
                Subprocess.INPUT_FLAG
            ])
        return subproc.output

    pass