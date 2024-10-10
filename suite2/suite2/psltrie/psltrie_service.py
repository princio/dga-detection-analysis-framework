

import csv
from lib2to3.pgen2.parse import ParseError
import logging
from os import error
from pathlib import Path
from tempfile import NamedTemporaryFile, TemporaryFile
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

        with NamedTemporaryFile('w', delete=False) as inputfile:
            input.to_csv(inputfile, index=False, doublequote=False, quoting=csv.QUOTE_NONE)
            inputfile.flush()
            output = self.subprocess_service.launch_psltrie(
                Path(inputfile.name),
                self.psl_list_service._listfile
            )
            logging.getLogger(__name__).debug('input %s' % inputfile.name)
            logging.getLogger(__name__).debug(output)
            pass

        return pd.read_csv(output)


        