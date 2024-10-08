import logging
from pathlib import Path
from ..subprocess import Subprocess

class TCPDumpService:
    def __init__(self, binary: Path, workdir: str) -> None:
        self.name = 'tcpdump'
        self.binary = binary
        self.workdir = Path(workdir).joinpath('tcpdump')

        if not self.workdir.exists():
            self.workdir.mkdir(exist_ok=True)
            pass

        self.tcpdump = Subprocess(logging.getLogger(), self.name, self.binary, self.workdir)
        pass

    def run(self, pcapfile: Path):
        self.tcpdump.launch(
            pcapfile, [
                "-r", Subprocess.INPUT_FLAG,
                "-w", Subprocess.OUTPUT_FLAG,
                "port", "53"
            ])
        return self.tcpdump.output

    pass