
from pathlib import Path
import numpy as np
import pandas as pd

from ..dn.dn_service import DBDNService
from ..db import Database


class MessageService:
    def __init__(self, db: Database, dbdn_service: DBDNService):
        self.db = db
        self.dbdn_service = dbdn_service
        pass

    def _insert_pcap(self, pcap_id: int, pcapcsvfile: Path):
        df = pd.read_csv(pcapcsvfile)

        df['server'] = df['server'].replace([np.nan], [None])
        df['answer'] = df['answer'].replace([np.nan], [None])

        df = df.reset_index(names='fn')

        df['dn_id'] = self.dbdn_service.add(df["dn"])

        with self.db.psycopg2().cursor() as cursor:
            df["pcap_id"] = pcap_id
            df["is_r"] = df["qr"] == "r"

            values = df[["fn", "pcap_id", "time", "fn", "fnreq", "dn_id", "qcode", "is_r", "rcode", "server", "answer"]].to_numpy().tolist()
            
            args_str = b",".join([cursor.mogrify("(%s, %s,%s,%s,%s, %s, %s, %s,%s,%s, %s)", x) for x in values])

            cursor.execute(f"""
                INSERT INTO public.message_{pcap_id}(
                    id, pcap_id, time_s, fn,
                    fn_req, dn_id, qcode,
                    is_r, rcode, server,
                    answer)
                VALUES""" + args_str.decode("utf-8") + 
                " ON CONFLICT DO NOTHING")
            
            # cursor.connection.commit()
            cursor.close()
            pass
        
        return df
    

    def insert_pcapcsvfile(self, pcap_id: int, pcapcsvfile: Path):
        self._insert_pcap(pcap_id, pcapcsvfile)

        pass
        