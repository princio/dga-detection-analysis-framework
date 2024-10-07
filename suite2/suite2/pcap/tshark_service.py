from dataclasses import dataclass
from pathlib import Path

from ..utils import Subprocess

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

        print(f"[tshark]: {self.workdir}")

        if not self.workdir.exists():
            self.workdir.mkdir(exist_ok=True)
            pass
        pass

    def run(self, pcapfile: Path):
        workdir = self.workdir.joinpath(pcapfile.name)

        subproc = Subprocess(self.name, self.binary, workdir)

        subproc.launch(
            pcapfile, [
                "-r", Subprocess.INPUT_FLAG,
                "-Y", "dns && !_ws.malformed && !icmp",
                "-w", Subprocess.OUTPUT_FLAG
            ])

        return subproc.output

    pass