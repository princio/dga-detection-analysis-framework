

from pathlib import Path
import pandas as pd
from ..utils import Subprocess
from ..psl_list.psl_list_service import PSLListService

class PSLTrieService:
    def __init__(self, psl_list_service: PSLListService, binary: Path, workdir: Path):
        self.binary = binary
        self.psl_list_service = psl_list_service

        self.workdir: Path = Path(workdir).joinpath('psltrie')
        if not self.workdir.exists():
            self.workdir.mkdir(exist_ok=True)

        self.psltrie = Subprocess('psltrie', binary, self.workdir)

        pass

    def run(self, input: pd.Series) -> pd.DataFrame:
        """Return the dataframe output of prsltrie.

        Args:
            input (pd.Series): A pandas Series containing a list of domain names.

        Returns:
            pd.DataFrame: A dataframe with the following columns:
            dn, bdn, rcode, tld, icann, private, dn_tld, dn_icann, dn_private.
            Missing suffixes has np.NaN value.
        """
        input_path = self.workdir.joinpath('s_dn.csv')
        input.to_csv(input_path, index=False)

        self.psl_list_service.run()

        self.psltrie.launch(
            input_path, [
                self.psl_list_service._listfile,
                "csv",
                "0",
                Subprocess.INPUT_FLAG,
                Subprocess.OUTPUT_FLAG
            ])
        
        return pd.read_csv(self.psltrie.output)


        