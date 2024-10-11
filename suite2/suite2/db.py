
from dataclasses import dataclass
import logging
from math import ceil

import pandas as pd
import psycopg2
from sqlalchemy import create_engine

class Database:
    # uri = f"postgresql+psycopg2://postgres:@localhost:5432/dns_mac"
    def __init__(self, env, host, user, password, dbname, port=5432):
        self.dev : bool = env == "debug"
        self.host = host
        self.user = user
        self.password = password
        self.dbname = dbname
        self.port = port
        
        self._psyconn = None
        pass
    
    def psycopg2(self):
        if self._psyconn is None or self._psyconn.closed:
            self._psyconn = psycopg2.connect("host=%s dbname=%s user=%s password=%s" % (self.host, self.dbname, self.user, self.password))
        return self._psyconn
    
    def commit(self, connection):
        if self.dev:
            logging.getLogger(__name__).debug("Commit disabled: environment is «debug».")
        else:
            connection.commit()
        pass
    
    def sqlalchemy(self):
        return create_engine(f"postgresql+psycopg2://{self.user}:@{self.host}:{self.port}/{self.dbname}")
    pass

    def fetch_one(self, cursor):
        data = cursor.fetchone()
        return None if data is None else data[0]

    def pandas_batching(self, df: pd.DataFrame, batch_size: int, fun, args):
        n_batches = ceil(df.shape[0] / batch_size)
        for batch in range(n_batches):
            batch_start = batch_size * batch
            batch_end = batch_size * (batch + 1) if (batch + 1) < n_batches else df.shape[0]
            df_batch = df.iloc[batch_start:batch_end]
            fun(df_batch, args)
            pass