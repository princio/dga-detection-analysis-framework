
import logging
from tempfile import NamedTemporaryFile
import numpy as np
import pandas as pd

from ..dn.dn_service import DBDNService
from ..db import Database


class MessageService:
    INSERT_COLS = [ "id", "pcap_id", "time_s", "fn", "fn_req", "dn_id", "qcode", "is_r", "rcode", "server", "answer" ]
    def __init__(self, db: Database, dbdn_service: DBDNService):
        self.db = db
        self.dbdn_service = dbdn_service
        pass

    def _insert_into(self, df, cursor, partition):
        values = df[MessageService.INSERT_COLS].to_numpy().tolist()
        args_str = b",".join([cursor.mogrify("(%s, %s,%s,%s,%s, %s, %s, %s,%s,%s, %s)", x) for x in values])
        cursor.execute(f"""
            INSERT INTO public.message_{partition}
            ({','.join(MessageService.INSERT_COLS)})
            VALUES {args_str.decode('utf-8')}""")
        pass

    def _copy(self, df, cursor, partition):
        df_message = df[MessageService.INSERT_COLS]
        with NamedTemporaryFile('w') as tmpf:
            print(tmpf.name)
            df_message.to_csv(tmpf, index=False)
            cursor.execute(f"""
                COPY public.message_{partition} ({','.join(MessageService.INSERT_COLS)})
                FROM '{tmpf.name}'
                DELIMITER ','
                CSV HEADER;"""
            )
            pass
        pass

    def _insert(self, partition, df: pd.DataFrame, how):
        df = df.sort_values(by='time_s').reset_index(names='fn')
        df['id'] = df['fn']
        with self.db.psycopg2().cursor() as cursor:
            logging.info(f"MessageService inserts {df.shape[0]} rows in message_{partition} corresponding to {df.memory_usage(index=False).sum() / (1024 ** 2)} MB.")
            if how == "insert":
                self._insert_into(df, cursor, partition)
            elif how == "copy":
                self._copy(df, cursor, partition)
            else:
                raise Exception('Only `insert` and `copy` are valid.')
                pass
            self.db.commit(cursor.connection)
            pass
        pass

    def dns_parse_preprocess(self, df: pd.DataFrame, pcap_id: int) -> pd.DataFrame:
        df['server'] = df['server'].replace([np.nan], [None])
        df['answer'] = df['answer'].replace([np.nan], [None])
        df['dn_id'] = self.dbdn_service.add(df["dn"])
        df["is_r"] = df["qr"] == "r"
        df["pcap_id"] = pcap_id
        return df.rename(columns={'time': 'time_s', 'fnreq': 'fn_req'})

    def create_partition(self, partition, partition_ids):
        with self.db.psycopg2().cursor() as cursor:
            cursor.execute(
                f"""
                CREATE TABLE IF NOT EXISTS public.message_{partition}
                PARTITION OF public.message FOR VALUES IN
                ({','.join(map(str, partition_ids))});
                """
            )
            self.db.commit(cursor.connection)
            pass
        pass

    def insert(self, partition: str, df: pd.DataFrame):
        self._insert(partition, df, 'insert')
        pass

    def copy(self, partition: str, df: pd.DataFrame):
        self._insert(partition, df, 'copy')
        pass
