

import logging
from pathlib import Path
from typing import Optional, Tuple, Union, Any
import warnings
import numpy as np
import pandas as pd
from psycopg2.extras import execute_values

from suite2.psltrie.psltrie_service import PSLTrieService
from ..nn.nn_service import NNService
from ..defs import NN, NNType
from ..db import Database
from ..lstm.lstm_service import LSTMService

class DNService:
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

        logging.getLogger(__name__).debug(f"Adding {uniques.shape[0]} domain names.")
        with self.db.psycopg2().cursor() as cursor:
            args_str = b",".join([ cursor.mogrify("(%s)", (x,)) for x in uniques ])
            cursor.execute(b"""
                INSERT INTO public.dn(
                    dn)
                    VALUES """ +
                args_str +
                b" ON CONFLICT DO NOTHING RETURNING id")
        
            logging.getLogger(__name__).info(f"Added {cursor.rowcount} of {uniques.shape[0]} domain names.")

            if cursor.rowcount < uniques.shape[0]:
                tuples = tuple( item for item in uniques )
                cursor.execute("""SELECT id from dn WHERE dn.dn IN %s""", (tuples, ))
                pass
            
            ids = [t[0] for t in cursor.fetchall()]

            self.db.commit(cursor.connection)
            pass

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

    def db_lstm(self, nn: NN, batch_size = 100_000):
        model = self.lstm_service.load_model(nn.model_json, Path(nn.hf5_file.name))
        todo = self.db_lstm_to_do(nn.id)
        if todo == 0:
            return
        
        todo_done = 0
        with self.db.psycopg2().cursor() as cursor:
            cursor.execute("""SELECT DN.ID AS ID, DN, TLD, ICANN, PRIVATE, NN_ID FROM DN LEFT JOIN DN_NN ON (DN.ID = DN_NN.DN_ID AND DN_NN.NN_ID = %s)  WHERE NN_ID is null""", (nn.id,))
            while True:
                rows = cursor.fetchmany(batch_size)
                logging.getLogger(__name__).info(f'Processing with {nn.name}#{nn.id} {len(rows)} over {todo} domain names.')
                if not rows:
                    break

                df = pd.DataFrame.from_records(rows, columns=["id", "dn", "tld", "icann", "private", "nn_id"])
                
                _, Y = self.run(df['dn'], (nn.nntype, model), df)

                df["Y"] = Y
                with warnings.catch_warnings():
                    df["logit"] = np.log(Y / (1 - Y))
                
                with self.db.psycopg2().cursor() as cursornn:
                    args_str = b",".join([cursor.mogrify("(%s, %s, %s, %s)", (row["id"], nn.id, row["Y"], row["logit"])) for _, row in df.iterrows()])
                    cursornn.execute(
                        b"""INSERT INTO public.dn_nn(dn_id, nn_id, value, logit) VALUES """ + args_str
                    )
                    pass
                self.db.commit(cursor.connection)
                todo_done += len(rows)
                logging.getLogger(__name__).info(f'Processed with {nn.name}#{nn.id} {todo_done}/{todo} domain names.')
                pass # while true
            pass
        pass

    def db_psltrie_to_do(self):
        with self.db.psycopg2().cursor() as cursor:
            cursor.execute("""SELECT COUNT(*) FROM DN WHERE DN.PSLTRIE_RCODE IS NULL""")
            todo = cursor.fetchone()
            if todo is None:
                raise Exception('Query failed.')
            pass
        return todo[0]
    
    def db_psltrie(self, batch_size=100_000):
        todo = self.db_psltrie_to_do()
        logging.getLogger(__name__).info(f'The number of domains to be psltrie-processed are: {todo}.')
        if todo == 0:
            return
        
        todo_done = 0
        with self.db.psycopg2().cursor() as cursor:
            cursor.execute("SELECT ID, DN FROM DN WHERE DN.PSLTRIE_RCODE IS NULL ORDER BY ID")
            while True:
                rows = cursor.fetchmany(batch_size)
                if not rows:
                    if todo_done < todo:
                        raise Exception(f'Missing {todo - todo_done} over {todo} domain names to be psltrie-processed.')
                    break
                logging.getLogger(__name__).debug(f'Processing with psltrie {len(rows)} over {todo} domains.')

                df = self.psltrie_service.run(pd.Series([ row[1] for row in rows ]))
                df['id'] = [ row[0] for row in rows ]

                df.rename(columns={ "rcode": "psltrie_rcode", "is_valid": "psltrie_isvalid" }, inplace=True)
                df = df.replace(np.nan, None)
                df['psltrie_isvalid'] = df['psltrie_isvalid'].astype(bool)

                cols = ['bdn','tld','icann','private','psltrie_rcode','psltrie_isvalid']
                sets = ", ".join([ f"{col}=data.{col}" for col in cols ])

                with self.db.psycopg2().cursor() as update_cursor:
                    sql = f"""UPDATE dn SET {sets} FROM (VALUES %s) AS data ({','.join(cols)},id) WHERE dn.id = data.id;"""
                    values = df[cols + [ "id" ]].values.tolist()
                    execute_values(update_cursor, sql, values)
                    self.db.commit(update_cursor.connection)
                    pass

                todo_done += len(rows)
                logging.getLogger(__name__).info(f'Processed {todo_done}/{todo} domain names.')
                pass # while true
            pass # cursor
        pass # for nns

    def db_dgarchive(self):
        """UPDATE dn SET dac_id = dac.id FROM dac WHERE dac.dn=dn.dn"""
        pass

    def dbfill(self):
        self.db_psltrie()
        for nn in self.nn_service.get_all():
            self.db_lstm(nn)
            pass
        pass
