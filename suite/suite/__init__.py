import argparse
from pathlib import Path

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


    from dn import DN
    dn = DN(config)
    dn.run()
    
    from lstm import LSTM
    lstm = LSTM(config)
    lstm.run()

    from whitelisting import Whitelisting
    for whitelist in config.whitelists:
        wl = Whitelisting(config, whitelist)
        wl.run_add()
        wl.run_merge_dn()
        pass

    pass