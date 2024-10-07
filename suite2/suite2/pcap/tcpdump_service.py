from pathlib import Path
from ..utils import Subprocess

class TCPDumpService:
    def __init__(self, binary: Path, workdir: str) -> None:
        self.name = 'tcpdump'
        self.binary = binary
        self.workdir = Path(workdir).joinpath('tcpdump')

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
                "-w", Subprocess.OUTPUT_FLAG,
                "port", "53"
            ])
        return subproc.output

    pass