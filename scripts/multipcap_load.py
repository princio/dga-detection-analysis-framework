import logging
from pathlib import Path
import subprocess
import sys
from tempfile import NamedTemporaryFile, TemporaryFile
from dependency_injector.wiring import Provide, inject
import pandas as pd
import psycopg2

sys.path.append(str(Path(__file__).resolve().parent.parent.joinpath('suite2').absolute()))
from suite2.psltrie.psltrie_service import PSLTrieService
from suite2.defs import NNType
from suite2.dn.dn_service import DNService
from suite2.lstm.lstm_service import LSTMService
from suite2.nn.nn_service import NNService
from suite2.pcap.pcap_service import PCAPService
from suite2.container import Suite2Container

@inject
def test_pcap(
        pcap_service: PCAPService = Provide[
            Suite2Container.pcap_service
        ],
) -> None:
    for day in range(3,6):
        try:
            pcapfiles = list(Path(f'/Users/princio/Downloads/IT2016/Day{day}').glob('./*.pcap'))
            pcapfiles = pd.Series(pcapfiles).sort_values().to_list()
            pcap_service.new_partition(pcapfiles, f'it16_{day}')
        except Exception as e:
            print(e)
    return

@inject
def test_lstm(
        dn_service: DNService = Provide[
            Suite2Container.dn_service
        ],
        lstm_service: LSTMService = Provide[
            Suite2Container.lstm_service
        ],
        nn_service: NNService = Provide[
            Suite2Container.nn_service
        ],
) -> None:
    nns = nn_service.get_all()

    for nn in nns:
        nn = nn_service.get_model(nn.id)

        model = lstm_service.load_model(nn.model_json, Path(nn.hf5_file.name))

        df_dn = dn_service.get(1000, nn.id)

        dn_rev, Y = dn_service.run(df_dn['dn'], (nn.nntype, model), df_dn)

        df_dn['rev'] = dn_rev
        df_dn['Y'] = Y

        y1 = (df_dn['eps'] - df_dn['Y'])
        y2 = (df_dn['eps'] - df_dn['Y']).round(3) * 1e4
        y2 = y2.astype(int)
        mask = (0 == y2)

        print(mask.all(), mask.sum()) # type: ignore
        if not mask.all(): # type: ignore
            df_dn['y1'] = y1
            df_dn['y2'] = y2
            print(df_dn[~mask].to_markdown())
        pass
    pass

if __name__ == "__main__":

    application = Suite2Container()
    
    application.config.from_dict({
        "env": "prod",
        "db": {
            "host": "localhost",
            "user": "postgres",
            "password": "postgre",
            "dbname": "dns_mac",
            "port": 5432
        },
        "binaries": {
            "tshark": "/opt/homebrew/bin/tshark",
            "tcpdump": "/usr/sbin/tcpdump",
            "dns_parse": "/Users/princio/Repo/princio/malware-detection-predict-file/dns_parse/bin/dns_parse",
            "psltrie": "/Users/princio/Repo/princio/malware-detection-predict-file/psltrie/bin/binary_prod",
            "iconv": "/usr/bin/iconv"
        },
        "workdir": "/tmp/suite2_workdir",
        "logging": logging.INFO
    })

    application.init_resources()

    application.wire(modules=[__name__])

    test_pcap()

    application.shutdown_resources()

    pass