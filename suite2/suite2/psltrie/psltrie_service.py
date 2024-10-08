

import logging
from pathlib import Path
from tempfile import TemporaryFile
import pandas as pd
from ..subprocess.subprocess_service import SubprocessService
from ..psl_list.psl_list_service import PSLListService

class PSLTrieService:
    def __init__(self, subprocess_service: SubprocessService, psl_list_service: PSLListService, binary: Path, workdir: Path):
        self.binary = binary
        self.subprocess_service = subprocess_service
        self.psl_list_service = psl_list_service
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
        self.psl_list_service.run()

        with TemporaryFile('w') as inputfile:
            input.to_csv(inputfile, index=False)
            output = self.subprocess_service.launch_psltrie(
                Path(inputfile.name),
                self.psl_list_service._listfile
            )
            pass
        
        return pd.read_csv(output)


        