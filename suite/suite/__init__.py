from dataclasses import dataclass
import argparse
from pathlib import Path
from traceback import print_tb

import pandas as pd
from config import Config

from malware import Malwares
from pcap import PCAP

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
                    prog='DNSMalwareDetection',
                    description='Malware detection')

    parser.add_argument('configjson')
    parser.add_argument('workdir')

    args = parser.parse_args()

    configjson = Path(args.configjson)
    workdir = Path(args.workdir)

    config = Config(configjson, workdir)

    malwares = Malwares(config)

    for pcappath in config.pcaps:
        pcap = PCAP(config, malwares, pcappath)
        pcap.run()
        pass

    with config.psyconn.cursor() as cursor:
        df = pd.read_sql("""select * from dn where tld is null""", config.sqlalchemyconnection)
        if df.shape[0] > 0:
            print("[warn]: TLD check failed, fix following DN to proceed the LSTM step.")
            print(df.to_markdown())
            pass
        pass

    from lstm import LSTM
    lstm = LSTM(config)
    lstm.run()

    for whitelist in config.whitelists:
        print(whitelist)

        df = pd.read_csv(whitelist.path, index_col=False)

        if not whitelist.bdn_col in df:
            df[whitelist.bdn_col] = df[whitelist.dn_col]
            pass

        if not whitelist.rank_col == "#index":
            df.index.set_names("rank")
            df.reset_index(inplace=True)
            pass
        elif not whitelist.rank_col in df:
            df[whitelist.rank_col] = 1

        values = [ [ x["dn"], x["bdn"], x["rank"] ] for _, x in df.iterrows() ]
        args_str = b",".join([cursor.mogrify("(%s,%s,%s,%s,%s,%s)", x) for x in values])
        cursor.execute(b"""
            INSERT INTO public.dn(
                dn, bdn, tld, icann, private, invalid)
                VALUES """ +
            args_str +
            b" ON CONFLICT DO NOTHING RETURNING id")
        
        values = [ [ x["dn"], x["bdn"], x["tld"], x["icann"], x["private"], not bool(x["is_valid"]) ] for _, x in df_u.iterrows() ]
        args_str = b",".join([cursor.mogrify("(%s,%s,%s,%s,%s,%s)", x) for x in values])
        cursor.execute(b"""
            INSERT INTO public.dn(
                dn, bdn, tld, icann, private, invalid)
                VALUES """ +
            args_str +
            b" ON CONFLICT DO NOTHING RETURNING id")

        pass


    pass