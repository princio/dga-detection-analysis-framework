from typing import List
import numpy as np
import pandas as pd
from psycopg2.extras import execute_values


from config import Config
from utils import subprocess_launch

class DN:
    def __init__(self, config: Config) -> None:
        self.config = config
        self.batch_size = 10_000
        pass

    def select_by_dn(self, dn_s: List[str]):

        cursor = self.config.psyconn.cursor()

        tuples = tuple( x for x in dn_s )
        cursor.execute("""SELECT * FROM dn WHERE dn.dn IN %s""", (tuples, ))

        return cursor.fetchall()

    def run(self):

        cursor = self.config.psyconn.cursor()
        cursor.execute("""SELECT ID, DN FROM DN WHERE DN.psltrie_rcode is null""")

        while True:
            rows = cursor.fetchmany(self.batch_size)
            if not rows:
                break

            csv_in = self.config.workdir.path.joinpath("dn.in.csv")
            csv_out = self.config.workdir.path.joinpath("dn.out2.csv")
            df = pd.DataFrame.from_records(rows, columns=["id", "dn"]).to_csv(csv_in, index=False)

            subprocess_launch(self.config, None, "dn", [
                self.config.bin.psltrie,
                self.config.workdir.psl_list_path,
                "csv",
                "1",
                csv_in,
                csv_out
            ])

            df_in = pd.read_csv(csv_in, index_col=False)
            df_out = pd.read_csv(csv_out, index_col=False)

            df = pd.concat([ df_in["id"], df_out ], axis=1)

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
            with self.config.psyconn.cursor() as cursor2:
                execute_values(cursor2, sql, values)
                cursor2.connection.commit()
            pass # while true
        pass # for nns
    pass # def