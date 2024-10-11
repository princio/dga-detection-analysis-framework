


import argparse


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
                    prog='LSTM Processer',
                    description='Author: Lorenzo Principi, 2024')

    parser.add_argument('pcaplist', help='The newline separated file containing the PCAP.')
    parser.add_argument('outdir', help='The directory where all files are saved.')

    