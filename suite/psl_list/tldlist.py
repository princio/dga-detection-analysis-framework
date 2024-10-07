
from pathlib import Path
from typing import Union
import pandas as pd
import requests
from io import StringIO

from .common import TLDLIST_COLUMNS, TLDLIST_FIELDS, ETLDType, Origin
from .psl import ETLD
TYPE_CONVERTER = {
    "gTLD": ETLDType.generic,
    "ccTLD": ETLDType.country_code,
    "sTLD": ETLDType.sponsored,
    "infrastructure": ETLDType.infrastructure,
    "grTLD": ETLDType.generic_restricted,
    "test": ETLDType.test
}

class TLDLIST:
    SUFFIX_COLUMN = "TLD"
    TYPE_COLUMN = "Type"

    def __init__(self, dir: Path) -> None:
        self.dir: Path = dir
        self.file: Path = dir.joinpath(f"{self.__class__.__name__}.csv")
        self.index = 0
        pass

    def download(self, force) -> Union[pd.DataFrame,None]:
        self.dir.mkdir(parents=True, exist_ok=True)
        if force or not self.file.exists():
            url = "https://tld-list.com/df/tld-list-details.csv"
            headers = {"User-Agent": "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.14; rv:66.0) Gecko/20100101 Firefox/66.0"}
            req = requests.get(url, headers=headers)
            data = StringIO(req.text)
            df = pd.read_csv(data)
            df.to_csv(self.file)
            pass
        else:
            df = pd.read_csv(self.file, index_col=self.index)
            pass

        df_columns = [ col for col in df.columns if not col in [ self.SUFFIX_COLUMN ] ]

        for i, col in enumerate(TLDLIST_COLUMNS):
            if not df_columns[i] == col[2]:
                print(f"{self.__class__.__name__} columns not corresponds: {df.columns[i]} <> {col[2]}")
                return None
        
        return df

    def parse(self, etld: ETLD, force=False):
        df = self.download(force)

        if df is None:
            return

        for _, row in df.iterrows():
            suffix: str = row[self.SUFFIX_COLUMN]

            row[self.TYPE_COLUMN] = TYPE_CONVERTER[row[self.TYPE_COLUMN]]

            fields = TLDLIST_FIELDS(**{
                field_name: row[col].replace("\n", "---") if type(row[col]) == str else row[col]
                for field_name,_,col in TLDLIST_COLUMNS
                })

            etld.add(suffix, Origin.tldlist, fields)

            pass
        
        return df


