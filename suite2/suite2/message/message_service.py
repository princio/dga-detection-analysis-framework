
import logging
from tempfile import NamedTemporaryFile
import numpy as np
import pandas as pd

from ..dn.dn_service import DNService
from ..db import Database


class MessageService:
    TABLE = 'message2'
    INSERT_COLS = [ "id", "pcap_id", "time_s", "fn", "fn_req", "dn_id", "qcode", "is_r", "rcode", "src", "dst", "answer" ]
    def __init__(self, db: Database):
        self.db = db
        pass

    def _insert_into(self, df, cursor, partition):
        values = df[MessageService.INSERT_COLS].to_numpy().tolist()
        args_str = b",".join([cursor.mogrify("(%s, %s,%s,%s,%s, %s, %s, %s,%s,%s, %s)", x) for x in values])
        cursor.execute(f"""
            INSERT INTO {MessageService.TABLE}_{partition}
            ({','.join(MessageService.INSERT_COLS)})
            VALUES {args_str.decode('utf-8')}""")
        pass

    def _copy(self, df, cursor, partition):
        df_message = df[MessageService.INSERT_COLS]
        with NamedTemporaryFile('w') as tmpf:
            df_message.to_csv(tmpf, index=False)
            cursor.execute(f"""
                COPY {MessageService.TABLE}_{partition} ({','.join(MessageService.INSERT_COLS)})
                FROM '{tmpf.name}'
                DELIMITER ','
                CSV HEADER;"""
            )
            pass
        pass

    def _insert(self, partition, df: pd.DataFrame, how):
        df = df.sort_values(by='time_s')
        df = df.reset_index(drop=True).reset_index(names='fn')
        df['id'] = df['fn']
        with self.db.psycopg2().cursor() as cursor:
            logging.getLogger(__name__).info(f"MessageService inserts {df.shape[0]} rows in message_{partition} corresponding to {df.memory_usage(index=False).sum() / (1024 ** 2)} MB.")
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
        df['src'] = df['src'].replace([np.nan], [None])
        df['dst'] = df['dst'].replace([np.nan], [None])
        df['answer'] = df['answer'].replace([np.nan], [None])
        df["is_r"] = df["qr"] == "r"
        df["pcap_id"] = pcap_id
        df['time_s'] = pd.to_datetime(df['time'] - df['time'].min(), unit='s').dt.strftime('%Y-%m-%d %H:%M:%S.%f')
        return df.rename(columns={'fnreq': 'fn_req'})

    def create_partition(self, partition, partition_ids):
        with self.db.psycopg2().cursor() as cursor:
            cursor.execute(
                f"""
                CREATE TABLE IF NOT EXISTS {MessageService.TABLE}_{partition}
                PARTITION OF {MessageService.TABLE} FOR VALUES IN
                ({','.join(map(str, partition_ids))});
                """
            )
            self.db.commit(cursor.connection)
            pass
        pass

    def exists(self, name: str):
        with self.db.psycopg2().cursor() as cursor:
            cursor.execute(f"SELECT EXISTS (SELECT FROM information_schema.tables WHERE table_schema='public' AND table_name='{name}');")
            return cursor.fetchone()[0] # type: ignore

    def count(self, name: str):
        with self.db.psycopg2().cursor() as cursor:
            try:
                cursor.execute(f"SELECT COUNT(*) FROM {name}")
            except Exception as e:
                cursor.connection.rollback()
                raise e
            return cursor.fetchone()[0] # type: ignore

    def insert(self, partition: str, df: pd.DataFrame):
        self._insert(partition, df, 'insert')
        pass

    def copy(self, partition: str, df: pd.DataFrame):
        self._insert(partition, df, 'copy')
        pass
