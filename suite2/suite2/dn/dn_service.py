

from pathlib import Path
import numpy as np
import pandas as pd
from psycopg2.extras import execute_values

from suite2.psltrie.psltrie_service import PSLTrieService
from ..nn.nn_service import NNService
from ..defs import NN, NNType
from ..db import Database
from ..lstm.lstm_service import LSTMService

class DBDNService:
    def __init__(self, db: Database, nn_service: NNService, lstm_service: LSTMService, psltrie_service: PSLTrieService):
        self.db = db
        self.nn_service = nn_service
        self.lstm_service = lstm_service
        self.psltrie_service = psltrie_service
        pass

    def add(self, dn: pd.Series) -> pd.Series:
        codes, uniques = dn.factorize()

        with self.db.psycopg2().cursor() as cursor:

            args_str = b",".join([ cursor.mogrify("(%s)", x) for x in uniques ])
            cursor.execute(b"""
                INSERT INTO public.dn(
                    dn)
                    VALUES """ +
                args_str +
                b" ON CONFLICT DO NOTHING RETURNING id")
        
            cursor.connection.commit()

        if cursor.rowcount < uniques.shape[0]:
            tuples = tuple( item for item in uniques )
            cursor.execute("""SELECT id from dn WHERE dn.dn IN %s""", (tuples, ))
            pass

        ids = [t[0] for t in cursor.fetchall()]

        cursor.connection.commit()
        cursor.close()

        return pd.Index(ids).take(codes).to_series()


    def lstm_to_do(self, nn_id: int):
        with self.db.psycopg2().cursor() as cursor:
            cursor.execute("""SELECT COUNT(DN.ID) FROM DN;""")
            dn_count = cursor.fetchone()
            if dn_count is None:
                raise Exception('Query failed.')

            cursor.execute("""SELECT COUNT(DN_NN.DN_ID) FROM DN_NN WHERE DN_NN.NN_ID=%s;""", (nn_id,))
            dn_nn_count = cursor.fetchone()
            if dn_nn_count is None:
                raise Exception('Query failed.')
            pass
        
        return dn_count[0] - dn_nn_count[0]


    def lstm(self, nn: NN, batch_size = 10_000):
        model = self.lstm_service.load_model(nn.model_json, Path(nn.hf5_file.name))
        count = self.lstm_to_do(nn.id)

        if count == 0:
            print("[info] no dn to be `lstm` processed.")
            return
        
        with self.db.psycopg2().cursor() as cursor:
            cursor.execute("""SELECT DN.ID, DN, TLD, ICANN, PRIVATE, NN_ID FROM DN LEFT JOIN DN_NN ON (DN.ID = DN_NN.DN_ID AND DN_NN.NN_ID = %s)  WHERE NN_ID is null""", (nn.id,))
            while True:
                rows = cursor.fetchmany(batch_size)
                if not rows:
                    break

                df = pd.DataFrame.from_records(rows, columns=["id", "dn", "tld", "icann", "private", "nn_id", "psltrie_rcode"])
                
                suffixes = None 
                if nn.nntype is not None:
                    df["icann"] = df["icann"].fillna(value=df["tld"])
                    df["private"] = df["private"].fillna(value=df["icann"])
                    suffixes = df[nn.nntype.name.lower()]
                    pass
            
                _, Y = self.lstm_service.run(model, df['dn'], suffixes)

                df["Y"] = Y
                df["logit"] = np.log(Y / (1 - Y))
                
                with self.db.psycopg2().cursor() as cursor:
                    args_str = b",".join([cursor.mogrify("(%s, %s, %s, %s)", (row["id"], nn.id, row["Y"], row["logit"])) for _, row in df.iterrows()])
                    cursor.execute(
                        b"""
                        INSERT INTO public.dn_nn(dn_id, nn_id, value, logit)
                            VALUES """ +
                            args_str
                    )
                    cursor.connection.commit()
                pass # while true
            pass

        print("Done %d dn for %s." % (count, nn.name))
        pass

    def psltrie(self, batch_size=10_000):
        psyconn = self.db.psycopg2()
        cursor = psyconn.cursor()

        cursor.execute("""SELECT ID, DN FROM DN WHERE DN.psltrie_rcode is null""")

        while True:
            rows = cursor.fetchmany(batch_size)
            if not rows:
                break

            df = pd.DataFrame.from_records(rows, columns=["id", "dn"])

            df_out = self.psltrie_service.run(df['dn'])

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
