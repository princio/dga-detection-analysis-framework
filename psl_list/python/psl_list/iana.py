
from dataclasses import make_dataclass
from pathlib import Path
from typing import Union
import pandas as pd

from pslregex.common import IANA_COLUMNS, IANA_FIELDS, ETLDType,  Origin
import pslregex.etld

IANA_COLUMNS = [
        ("type", ETLDType, "Type"),
        ("tld_manager", str, "TLD Manager")
    ]

TYPE_CONVERTER = {
    "generic": ETLDType.generic,
    "country-code": ETLDType.country_code,
    "sponsored": ETLDType.sponsored,
    "infrastructure": ETLDType.infrastructure,
    "generic-restricted": ETLDType.generic_restricted,
    "test": ETLDType.test
}

class IANA:
    SUFFIX_COLUMN = "Domain"
    TYPE_COLUMN = "Type"

    def __init__(self, dir: Path) -> None:
        self.dir = dir
        self.file: Path = dir.joinpath(f"{self.__class__.__name__.lower()}.csv")
        self.index = 0
        pass

    def download(self, force) -> Union[pd.DataFrame,None]:
        if force or not self.file.exists():
            df = pd.read_html('https://www.iana.org/domains/root/db', attrs = {'id': 'tld-table'})[0]
            df.to_csv(self.file)
            pass
        else:
            df = pd.read_csv(self.file, index_col = self.index)
            pass

        df_columns = [ col for col in df.columns if not col in [ self.SUFFIX_COLUMN ] ]

        for i, col in enumerate(IANA_COLUMNS):
            if not df_columns[i] == col[2]:
                print(f"{self.__class__.__name__} columns not corresponds: {df.columns[i]} <> {col[2]}")
                return None
        
        return df
    
    def parse(self, etld: pslregex.etld.ETLD, force=False) -> Union[pd.DataFrame,None]:

        df = self.download(force)

        if df is None:
            return None
        

        for _, row in df.iterrows():
            suffix: str = row[self.SUFFIX_COLUMN]

            row[self.TYPE_COLUMN] = TYPE_CONVERTER[row[self.TYPE_COLUMN]]

            suffix = suffix.replace('.', '', 1)
            suffix = suffix.replace('\u200f', '', 1)
            suffix = suffix.replace('\u200e', '', 1)
    
            if suffix.count('.') > 1:
                raise Exception('Unexpected: TLDs should have only one point each.')
            
            fields = IANA_FIELDS(**{
                field_name: row[col].replace("\n", "--") if type(row[col]) == str else row[col]
                for field_name,_,col in IANA_COLUMNS
                })
            
            etld.add(suffix, Origin.iana, fields)
        
        return df
    pass
pass