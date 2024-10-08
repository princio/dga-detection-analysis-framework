from dataclasses import dataclass
import logging
from pathlib import Path

from ..subprocess import Subprocess

@dataclass
class PCAPBinaries:
    dns_parse: Path = Path()
    tcpdump: Path = Path()
    tshark: Path = Path()
    pass

class TSharkService:
    def __init__(self, binary: Path, workdir: Path) -> None:
        self.name = 'tshark'
        self.binary = binary
        self.workdir = Path(workdir).joinpath('tshark')

        if not self.workdir.exists():
            self.workdir.mkdir(exist_ok=True)
            pass
        pass

    def run(self, pcapfile: Path):
        workdir = self.workdir.joinpath(pcapfile.name)

        subproc = Subprocess(logging.getLogger(), self.name, self.binary, workdir)

        subproc.launch(
            pcapfile, [
                "-r", Subprocess.INPUT_FLAG,
                "-Y", "dns && !_ws.malformed && !icmp",
                "-w", Subprocess.OUTPUT_FLAG
            ])

        return subproc.output

    pass