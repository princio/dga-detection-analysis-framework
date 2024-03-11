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
            print("TLD check failed: fix following DN to proceed the LSTM step.")
            print(df.to_markdown())
            exit(1)
        else:
            print("TLD check: success.")
        pass

    from lstm import LSTM
    lstm = LSTM(config)
    lstm.run()


    pass