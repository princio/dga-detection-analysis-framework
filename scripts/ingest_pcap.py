import logging
from pathlib import Path
import subprocess
import sys
from tempfile import NamedTemporaryFile, TemporaryFile
from dependency_injector.wiring import Provide, inject
import pandas as pd
import psycopg2

sys.path.append(str(Path(__file__).resolve().parent.parent.joinpath('suite2').absolute()))
from suite2.pcap import pcap_service
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
    path = Path('/Users/princio/Downloads/IT2016/Day0/20160424_055409.pcap')
    o = pcap_service.process(path)

    pcap_id = pcap_service.insert(path, 'asasas', 2000, 'tmp')

    # pcap_service.message_service.create_partition('tmpp', [pcap_id])

    df = pd.read_csv(o)
    df['dn_id'] = pcap_service.dn_service.add(df["dn"]) # important
    df = pcap_service.message_service.dns_parse_preprocess(df, pcap_id)
    pcap_service.message_service.copy('tmp', df)

    return


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