

import pandas as pd
from .utils import Subprocess
from .psl_list import PSLList

class PSLTrie:
    def __init__(self, binary, psllist: PSLList, outdir):
        self.binary = binary
        self.psllist = psllist
        self.outdir = outdir

        self.psltrie = Subprocess('psltrie', binary, self.outdir)

        pass

    def load(self) -> pd.DataFrame:
        return pd.read_csv(self.psltrie.output)

    def run(self, input: pd.Series):
        if not self.psllist.exists():
            self.psllist.run()
            pass

        input_path = self.outdir.joinpath('s_dn.csv')
        input.to_csv(input_path, index=False)

        self.psltrie.launch(
            input_path, [
                self.psllist.listpath,
                "csv",
                "0",
                Subprocess.INPUT_FLAG,
                Subprocess.OUTPUT_FLAG
            ])
        pass


        