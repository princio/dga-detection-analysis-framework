from typing import List
import numpy as np
import pandas as pd
from psycopg2.extras import execute_values

from suite.process.psl_list import PSLList


from ..process.psltrie import PSLTrie
from .common import Database

class DBPSLTrie:
    def __init__(self, db: Database, psltrie: PSLTrie, batch_size=10_000) -> None:
        self.db = db
        self.psltrie = psltrie
        self.batch_size = batch_size
        pass

    def select_by_dn(self, dn_s: List[str]):
        psycopg2 = self.db.psycopg2()
        cursor = psycopg2.cursor()

        tuples = tuple( x for x in dn_s )
        cursor.execute("""SELECT * FROM dn WHERE dn.dn IN %s""", (tuples, ))

        return cursor.fetchall()

    def run(self):
        psyconn = self.db.psycopg2()
        cursor = psyconn.cursor()

        cursor.execute("""SELECT ID, DN FROM DN WHERE DN.psltrie_rcode is null""")

        while True:
            rows = cursor.fetchmany(self.batch_size)
            if not rows:
                break

            df = pd.DataFrame.from_records(rows, columns=["id", "dn"])


            self.psltrie.run(df['dn'])
            df_out = self.psltrie.load()

            df = pd.concat([ df["id"], df_out ], axis=1)

            df.rename(columns={ "rcode": "psltrie_rcode" }, inplace=True)
            df = df.replace(np.nan, None)

            cols = "bdn,tld,icann,private,psltrie_rcode"
            sets = ", ".join([ f"{col}=data.{col}" for col in cols.split(",") ])
            values_cols = cols.split(",") + [ "id" ]
            values_names = cols + ",id"

            sql = """UPDATE dn SET """\
                    + sets\
                    + """ FROM (VALUES %s) AS data ("""\
                    + values_names\
                    + """) WHERE dn.id = data.id;"""

            values = df[values_cols].values.tolist()

            # PSLTRIE of TYPE TEXT
            with psyconn.cursor() as cursor2:
                execute_values(cursor2, sql, values)
                cursor2.connection.commit()
            pass # while true
        pass # for nns
    pass # def