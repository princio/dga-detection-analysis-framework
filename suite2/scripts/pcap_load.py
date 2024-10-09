"""Main module."""

import subprocess
from dependency_injector.wiring import Provide, inject
import sys
from pathlib import Path

import psycopg2

sys.path.append(str(Path(__file__).parent.parent.absolute()))

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
        dn_service: DNService = Provide[
            Suite2Container.dbdn_service
        ],
        lstm_service: LSTMService = Provide[
            Suite2Container.lstm_service
        ],
        nn_service: NNService = Provide[
            Suite2Container.nn_service
        ],
        psltrie_service: PSLTrieService = Provide[
            Suite2Container.psltrie_service
        ],
) -> None:
    
    it2016_day_partition = 'it16_0'
    pcapfile = Path("/Users/princio/Downloads/Day0/20160423_235403.pcap")

    sha256 = subprocess.getoutput(f"shasum -a 256 {pcapfile}").split(' ')[0]

    outputcsvfile = pcap_service.process(pcapfile)

    try:
        pcap_id = pcap_service.insert(pcapfile, sha256, 2016, 'IT16', partition=it2016_day_partition, infected=None)
    except psycopg2.errors.UniqueViolation as e:
        e.cursor.connection.rollback() # type: ignore
        pcap_id = pcap_service.findby_hash(sha256)
        pass

    pcap_service.message_service.insert_pcapcsvfile(pcap_id, it2016_day_partition, outputcsvfile)

    pass

@inject
def test_lstm(
        dn_service: DNService = Provide[
            Suite2Container.dbdn_service
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
        "env": "debug",
        "db": {
            "host": "localhost",
            "user": "postgres",
            "password": "postgre",
            "dbname": "dns_mac",
            "port": 5432
        },
        "tshark": "/opt/homebrew/bin/tshark",
        "tcpdump": "/usr/sbin/tcpdump",
        "dns_parse": "/Users/princio/Repo/princio/malware-detection-predict-file/dns_parse/bin/dns_parse",
        "psltrie": "/Users/princio/Repo/princio/malware-detection-predict-file/psltrie/bin/binary_prod",
        "workdir": "/tmp/suite2_workdir",
        "logging": 1
    })

    application.init_resources()

    application.wire(modules=[__name__])

    test_pcap()

    application.shutdown_resources()

    pass