import enum
from pathlib import Path
import tempfile
from typing import List
import numpy as np
import pandas as pd

from suite.process.lstm_univpm import lstm_univpm_run
from .common import Database

class DBModelType(enum.Enum):
    NONE = 0
    TLD = 1
    ICANN = 2
    PRIVATE = 3
    pass

class DBModel:
    def __init__(self, db: Database, id: int):
        self.db = db
        self.id = id
        self.name = ''
        self.extractor = DBModelType.NONE
        self.model_json = ''
        self.tempfile = tempfile.NamedTemporaryFile('wb', delete=False)
        self.fetched = False
    pass

    def fetch(self):
        if self.fetched:
            return
        with self.db.psycopg2().cursor() as cursor:
            cursor.execute("SELECT name, extractor, model_json, model_hf5 FROM nn WHERE id=%s", (self.id,))
            tmp = cursor.fetchone()
            if tmp is None:
                raise Exception('Query failed.')

            self.name = tmp[0]
            self.extractor = tmp[1]
            self.model_json = tmp[2]
            self.tempfile.write(tmp[3])
            self.fetched = True
            pass
        pass
    pass


def db_get_nns(db: Database) -> List[DBModel]:
    with self.db.psycopg2().cursor() as cursor:
        cursor.execute("""SELECT id FROM NN;""")
        ids = cursor.fetchall()
        if ids is None:
            raise Exception('Query failed.')
        pass
    return [ DBModel(self.db, id[0]) for id in ids ]

def dn_to_do(self, nn: DBModel):
    with self.db.psycopg2().cursor() as cursor:
        cursor.execute("""SELECT COUNT(DN.ID) FROM DN;""")
        dn_count = cursor.fetchone()
        if dn_count is None:
            raise Exception('Query failed.')

        cursor.execute("""SELECT COUNT(DN_NN.DN_ID) FROM DN_NN WHERE DN_NN.NN_ID=%s;""", (nn.id,))
        dn_nn_count = cursor.fetchone()
        if dn_nn_count is None:
            raise Exception('Query failed.')
        pass
    
    return dn_count[0] - dn_nn_count[0]



class DBDN:
    def __init__(self, db: Database, nn_type: DBModelType, batch_size = 10_000):
        self.db = db
        self.nn_type = nn_type
        self.cursor = None
        self.batch_size = batch_size
        pass

    def start(self):
        self.cursor = self.db.psycopg2().cursor()
        self.cursor.execute("""SELECT DN.ID, DN, TLD, ICANN, PRIVATE, NN_ID,
                            PSLTRIE_RCODE FROM DN LEFT JOIN DN_NN ON (DN.ID =
                            DN_NN.DN_ID AND DN_NN.NN_ID = %s)  WHERE NN_ID is
                            null""", (self.nn_type,))
        pass

    def next(self):
        if self.cursor is None:
            raise Exception('Before initiate with `start()`.')
        rows = self.cursor.fetchmany(self.batch_size)
        
        if not rows:
            return None

        df = pd.DataFrame.from_records(rows, columns=["id", "dn", "tld", "icann", "private", "nn_id", "psltrie_rcode"])

        if df["psltrie_rcode"].isna().sum() > 0:
            print(f'Warning: {df["psltrie_rcode"].isna().sum()} has not been processed.')
            pass

        df["icann"] = df["icann"].fillna(value=df["tld"])
        df["private"] = df["private"].fillna(value=df["icann"])

        return df

    pass



class DBLSTM:
    def __init__(self, db: Database, batch_size=10_000) -> None:
        self.db = db
        self.batch_size = batch_size
        pass

    def run(self, nn: DBModel):
        cursor = self.db.psycopg2().cursor()
        count = self.dn_to_do(nn)
        if count > 0:
            dbdn = DBDN(nn_type=nn.id, )

            while True:
                rows = cursor.fetchmany(self.batch_size)
                if not rows:
                    break

                df = pd.DataFrame.from_records(rows, columns=["id", "dn", "tld", "icann", "private", "nn_id", "psltrie_rcode"])

                if df["psltrie_rcode"].isna().sum() > 0:
                    print(f'Warning: {df["psltrie_rcode"].isna().sum()} has not been processed.')
                    pass

                df["icann"] = df["icann"].fillna(value=df["tld"])
                df["private"] = df["private"].fillna(value=df["icann"])

                s_dn = df['dn']
                if nn.extractor != DBModelType.NONE:
                    s_suffix = df[nn.extractor.name.lower()]
                    pass

                dn_reversed, Y = lstm_univpm_run(df['dn'], nn.model_json, Path(nn.tempfile.name))

                df["Y"] = Y
                df["logit"] = np.log(Y / (1 - Y))
                
                with self.db.psycopg2().cursor() as cursor:
                    args_str = b",".join([cursor.mogrify("(%s, %s, %s, %s)", (row["id"], nn.id, row["Y"], row["logit"])) for _, row in df.iterrows()])
                    cursor.execute(
                        b"""
                        INSERT INTO public.dn_nn(
                            dn_id, nn_id, value, logit)
                            VALUES """ +
                            args_str
                    )
                    cursor.connection.commit()
                pass # while true
            pass

        print("Done %d dn for %s." % (count, nn.name))
        pass

    def runall(self):
        nns = self.nns()
        for nn in nns:
            self.run(nn)
            pass
        pass
    pass
