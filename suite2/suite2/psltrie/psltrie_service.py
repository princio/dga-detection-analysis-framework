

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

    def run(self, s_dns: pd.Series) -> pd.DataFrame:
        """Return the dataframe output of prsltrie.

        Args:
            input (pd.Series): A pandas Series containing a list of domain names.

        Returns:
            pd.DataFrame: A dataframe with the following columns:
            dn, bdn, rcode, tld, icann, private, dn_tld, dn_icann, dn_private.
            Missing suffixes has np.NaN value.
        """
        self.psl_list_service.run()
        dns = s_dns.tolist()

        with NamedTemporaryFile('w', delete=False) as inputfile:
            logging.getLogger(__name__).critical('-----------%s' % inputfile)
            inputfile.write('0\n')
            inputfile.writelines([dn + '\n' for dn in dns])
            inputfile.flush()
            output = self.subprocess_service.launch_psltrie(
                Path(inputfile.name),
                self.psl_list_service._listfile
            )
            logging.getLogger(__name__).debug('input %s' % inputfile.name)
            logging.getLogger(__name__).debug(output)
            pass

        return pd.read_csv(output)


        