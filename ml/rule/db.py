'''
id, pcap_id, time_s, time_s_translated, fn, fn_req, dn_id,
dn, bdn, qcode, is_r, rcode, server, answer,
pcap_infected, pcap_malware_type, rn, rn_qr, rn_qr_rcode,
eps1, eps2, eps3, eps4, rank_dn, rank_bdn
'''

from dataclasses import dataclass
import enum

import pandas as pd
from sqlalchemy import create_engine


@dataclass
class Database:
    uri = f"postgresql+psycopg2://postgres:@localhost:5432/dns_mac"

    def __init__(self):
        self.engine = create_engine(self.uri)
        # self.conn = self.engine.connect()
        pass

def get_df(db: Database, mwname, pcap_id, g: bool, pt: str):

    if g:
        if pt == 'q':
            where = 'rn_qr=1 and is_r is false'
        elif pt == 'r':
            where = 'rn_qr=1 and is_r is true'
        elif pt == 'ok':
            where = 'rn_qr_rcode=1 and rcode = 0'
        elif pt == 'nx':
            where = 'rn_qr_rcode=1 and rcode = 3'
    else:
        if pt == 'q':
            where = 'is_r is false'
        elif pt == 'r':
            where = 'is_r is true'
        elif pt == 'ok':
            where = 'rcode = 0'
        elif pt == 'nx':
            where = 'rcode = 3'

    df = pd.read_sql(f"""
    SELECT time_s_translated, FLOOR(time_s_translated / 3600) as "hour" from public.get_message_pcap_all3(
        {pcap_id},
        0
    )
    where {where} and FLOOR(time_s_translated / 3600) < 24*30
    """, db.engine)

    df["p"] = 1

    df = df.groupby("hour").aggregate({"p": 'sum'})
    idx  = df.index.to_numpy().astype(int)

    index2 = []
    missing = []
    for i in range(24*30):
        if i not in idx:
            index2.append(i)
            missing.append(0)

    df.to_csv(f'{mwname}.csv')

    if len(missing) > 0:
        return pd.concat([df, pd.Series(missing, index2, name='p')]).sort_index()
    else:
        return df
