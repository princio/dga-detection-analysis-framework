
import copy
import enum
import json
from pathlib import Path
from typing import Literal, Union, cast

import numpy as np
import pandas as pd
from pandasgui import show

from defs import NN, Database, FetchConfig, pgshow
from slot2 import FetchField, FetchFieldMetric, sql_dataset

class Rule:
    def __init__(self, dir: Path, db: Database):
        self.db = db
        self.dir = dir
        pass

    def get_df(self, config: FetchConfig):
        fpath = self.dir.joinpath(f'{config.__hash__()}.csv')
        if Path(fpath).exists():
            df = pd.read_csv(fpath, index_col=0)
        else:
            with self.db.engine.connect() as conn:
                df = pd.read_sql(sql_dataset(config), conn)
                df.to_csv(fpath)
            pass
        df.slotnum = df.slotnum.astype(int)
        # print(df.slotnum.shape[0], df.slotnum.max() - df.slotnum.min() + 1)
        return df
    
    def _(self):
        
        pass