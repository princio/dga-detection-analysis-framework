from dataclasses import dataclass
import argparse
from pathlib import Path
from config import Config

from malware import Malwares
from pcap import PCAP
from lstm import LSTM

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

    lstm = LSTM(config)

    lstm.run()

    for pcappath in config.pcaps:
        pcap = PCAP(config, malwares, pcappath)
        pcap.run()
        pass

    pass