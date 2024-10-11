
import argparse
from dataclasses import dataclass
import json
from pathlib import Path

from ..process.pcap import PCAPProcess, PCAPBinaries

def process(binaries, input, outdir):
    ppp = PCAPProcess(binaries, input, outdir)
    df = ppp.run()
    df.to_csv(outdir.joinpath(input.with_suffix('.csv').name))
    pass

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
                    prog='PCAP Processer',
                    description='Author: Lorenzo Principi, 2024')
    

    parser.add_argument('binaries', help='''The .json file where the required binaries path are stored. Required binaries are: tcpdump, tshark, dns_parse.''')
    parser.add_argument('input', help='The .pcap file to processed.')
    parser.add_argument('outdir', help='The directory where the output file and the log are saved. If does not exist, it will be created.')

    args = parser.parse_args()

    with open(args.binaries) as fp:
        binaries_dict = json.load(fp)
        binaries = PCAPBinaries(
            dns_parse=binaries_dict['dns_parse'],
            tcpdump=binaries_dict['tcpdump'],
            tshark=binaries_dict['tshark']
        )
        pass
    
    input = Path(args.input)
    outdir = Path(args.outdir)

    process(binaries, input, outdir)

    pass
