
from dataclasses import dataclass
from math import ceil
from pathlib import Path
import subprocess
from typing import List, Union

import pandas as pd
import psycopg2
from sqlalchemy import create_engine

from config import Config

@dataclass
class PostgreURL:
    host: str
    user: str
    password: str
    dbname: str
    port=5432
    pass


@dataclass
class Database:
    # uri = f"postgresql+psycopg2://postgres:@localhost:5432/dns_mac"
    def __init__(self, postgreurl: PostgreURL):
        self.postgreurl = postgreurl
        self._psyconn = None
        pass
    
    def psycopg2(self):
        if self._psyconn is None or self._psyconn.closed:
            self._psyconn = psycopg2.connect("host=%s dbname=%s user=%s password=%s" % (self.postgreurl.host, self.postgreurl.dbname, self.postgreurl.user, self.postgreurl.password))
        return self._psyconn
    
    def sqlalchemy(self):
        return create_engine(f"postgresql+psycopg2://{self.postgreurl.user}:@{self.postgreurl.host}:{self.postgreurl.port}/{self.postgreurl.dbname}")
    pass




def fetch_one(cursor):
    data = cursor.fetchone()
    return None if data is None else data[0]

def subprocess_rename_file_if_error(test: bool, path_currentfile: Path):
    if test:
        path_errorfile = f"{path_currentfile}.error"
        if path_currentfile.exists():
            path_currentfile.rename(path_errorfile)
            pass
        pass
    pass

def subprocess_launch(config: Config, file_output: Union[Path,None], name, args: List):
    args = [ arg.absolute() if isinstance(arg, Path) else arg for arg in args ]

    if not file_output is None and file_output.exists():
        print(f"[info]: skipping {name}, file already exists.")
        return

    print(f"[info]: {name} step...")
    with open(config.workdir.log.joinpath(f"{name}.log"), "w") as fplog:
        try:
            p = subprocess.Popen(
                args,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
        except Exception as e:
            print(e)
            print("\n")
            print("\n".join([ str(arg) for arg in args ]))
            exit(1)

        output, errors = p.communicate(timeout=100000)

        fplog.writelines(output)
        fplog.writelines(errors)

        if not file_output is None:
            subprocess_rename_file_if_error(p.returncode != 0, file_output)

        if p.returncode != 0:
            print(output)
            print(errors)
            raise Exception(f"{name} execution failed")

        pass
    pass


def pandas_batching(df: pd.DataFrame, batch_size: int, fun, args):
    n_batches = ceil(df.shape[0] / batch_size)
    for batch in range(n_batches):
        batch_start = batch_size * batch
        batch_end = batch_size * (batch + 1) if (batch + 1) < n_batches else df.shape[0]
        print("Batching: %s/%s" % (batch+1, n_batches))

        df_batch = df.iloc[batch_start:batch_end]

        fun(df_batch, args)
        
        pass