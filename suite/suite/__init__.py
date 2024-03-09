from dataclasses import dataclass
import argparse
from pathlib import Path
from config import Config

from malware import Malwares
from pcap import PCAP

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
                    prog='DNSMalwareDetection',
                    description='Malware detection')

    parser.add_argument('configjson')
    parser.add_argument('workdir')
    parser.add_argument('pcapfile')

    args = parser.parse_args()

    configjson = Path(args.configjson)
    workdir = Path(args.workdir)
    pcapfile = Path(args.pcapfile)

    config = Config(configjson, workdir)

    malwares = Malwares(config)

    pcap = PCAP(config, malwares, pcapfile)

    pcap.run()

    pass