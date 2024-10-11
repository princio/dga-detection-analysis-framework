

from pathlib import Path
import pandas as pd

from suite import psl_list


class PSLList:
    def __init__(self, outdir: Path, output_name = 'psl_list.csv'):
        self.outdir: Path = outdir
        self.listpath: Path = outdir.joinpath(output_name)
        pass

    def exists(self):
        return self.listpath.exists()
    
    def load(self):
        return pd.read_csv(self.listpath)

    def run(self):
        self.outdir.mkdir(exist_ok=True, parents=True)

        if not self.exists():
            _etld = psl_list.etld.ETLD(self.outdir)
            _tldlist = psl_list.tldlist.TLDLIST(self.outdir)
            _iana = psl_list.iana.IANA(self.outdir)
            _psl = psl_list.psl.PSL(self.outdir)

            _tldlist.parse(_etld)
            _iana.parse(_etld)
            _psl.parse(_etld)

            _etld.to_csv(self.listpath)
            pass
        pass


        