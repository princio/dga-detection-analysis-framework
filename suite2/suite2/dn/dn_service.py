

from pathlib import Path
from typing import Optional, Tuple, Union, Any
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

    def get(self, limit: int, nn_id: int) -> pd.DataFrame:
        with self.db.psycopg2().cursor() as cursor:
            cursor.execute("SELECT DN.ID,DN,TLD,ICANN,PRIVATE, VALUE as EPS FROM DN JOIN (SELECT * FROM DN_NN WHERE NN_ID=%s) DN_NN ON (DN.ID=DN_NN.DN_ID) LIMIT %s", (nn_id, limit,))
            rows = cursor.fetchall()
            return pd.DataFrame(rows, columns=['id','dn','tld','icann','private','eps'])

    def add(self, dn: pd.Series) -> pd.Series:
        codes, uniques = dn.factorize()

        with self.db.psycopg2().cursor() as cursor:
            args_str = b",".join([ cursor.mogrify("(%s)", (x,)) for x in uniques ])
            cursor.execute(b"""
                INSERT INTO public.dn(
                    dn)
                    VALUES """ +
                args_str +
                b" ON CONFLICT DO NOTHING RETURNING id")
        
            if cursor.rowcount < uniques.shape[0]:
                tuples = tuple( item for item in uniques )
                cursor.execute("""SELECT id from dn WHERE dn.dn IN %s""", (tuples, ))
                pass
            
            ids = [t[0] for t in cursor.fetchall()]
            pass


        self.db.commit(cursor.connection)

        s = pd.Series(ids).take(codes) # type: ignore
        s.index = dn.index
        return s


    def run(self, dn_s: pd.Series, nn: Union[NN, Tuple[NNType, Any]], df_suffixes: Optional[pd.DataFrame] = None):
        """Execute psltrie and then run the nn-model.

        Args:
            nn (NN): The neural network entity.
            dn_s (pd.Series): A series containing domain names.
            df_suffixes (pd.DataFrame): A dataframe having columns
            ['tld','icann','private'] corresponding to the domain names in dn_s.

        Returns:
            (dn_reversed, Y): Output of LSTM module.
        """
        if isinstance(nn, NN):
            nntype = nn.nntype
            model = self.lstm_service.load_model(nn.model_json, Path(nn.hf5_file.name))
        else:
            nntype = nn[0]
            model = nn[1]
            pass

        if df_suffixes is None:
            df_suffixes = self.psltrie_service.run(dn_s)

        suffixes = None 
        if nntype != NNType.NONE:
            df_suffixes["icann"] = df_suffixes["icann"].fillna(value=df_suffixes["tld"])
            df_suffixes["private"] = df_suffixes["private"].fillna(value=df_suffixes["icann"])
            suffixes = df_suffixes[nntype.name.lower()]
            suffixes = suffixes.fillna('')
            pass
            
        return self.lstm_service.run(model, dn_s, suffixes)


    def db_lstm_to_do(self, nn_id: int):
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


    def db_lstm(self, nn: NN, batch_size = 10_000):
        model = self.lstm_service.load_model(nn.model_json, Path(nn.hf5_file.name))
        count = self.db_lstm_to_do(nn.id)

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
                
                _, Y = self.run(df['dn'], (nn.nntype, model), df)

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
                    self.db.commit(cursor.connection)
                pass # while true
            pass

        print("Done %d dn for %s." % (count, nn.name))
        pass

    def db_psltrie(self, batch_size=10_000):
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

            execute_values(cursor, sql, values)

            self.db.commit(cursor.connection)
            pass # while true
        pass # for nns
