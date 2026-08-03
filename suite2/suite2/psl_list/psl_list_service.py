

from pathlib import Path
import sys
import pandas as pd

# psl_list is a standalone component at the repository root, next to psltrie/
# which consumes the CSV it produces. Same sys.path idiom as scripts/*.py.
sys.path.append(str(Path(__file__).resolve().parents[3]))
import psl_list

class PSLListService:
    def __init__(self, workdir: Path):
        self.outdir: Path = Path(workdir).joinpath('psl_list')
        if not self.outdir.exists():
            self.outdir.mkdir(exist_ok=True)
        self._listfile: Path = Path(workdir).joinpath('psl_list.csv')
        pass

    def run(self):
        if not self._listfile.exists():
            etld = psl_list.etld.ETLD(self.outdir)
            tldlist = psl_list.tldlist.TLDLIST(self.outdir)
            iana = psl_list.iana.IANA(self.outdir)
            psl = psl_list.psl.PSL(self.outdir)

            tldlist.parse(etld)
            iana.parse(etld)
            psl.parse(etld)

            etld.to_csv(self._listfile)
        pass

    def load(self, listfile: Path) -> pd.DataFrame:
        if not listfile.exists():
            self.run()
            pass
        return pd.read_csv(listfile)


        