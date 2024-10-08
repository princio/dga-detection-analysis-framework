"""Main module."""

from dependency_injector.wiring import Provide, inject
import sys
from pathlib import Path

sys.path.append(str(Path(__file__).parent.parent.absolute()))

import pandas as pd



from suite2.psltrie.psltrie_service import PSLTrieService
from suite2.defs import NNType
from suite2.dn.dn_service import DBDNService
from suite2.lstm.lstm_service import LSTMService
from suite2.nn.nn_service import NNService
from suite2.pcap.pcap_service import PCAPService
from suite2.container import Suite2Container

@inject
def test_lstm(
        pcap_service: PCAPService = Provide[
            Suite2Container.pcap_service
        ],
        dn_service: DBDNService = Provide[
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
    pcapfile = Path("/Users/princio/Downloads/Day0/20160423_235403.pcap")
    
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

        print(mask.all(), mask.sum())
        if not mask.all():
            df_dn['y1'] = y1
            df_dn['y2'] = y2
            print(df_dn[~mask].to_markdown())
        pass
    pass

@inject
def main(
        pcap_service: PCAPService = Provide[
            Suite2Container.pcap_service
        ],
        dn_service: DBDNService = Provide[
            Suite2Container.dbdn_service
        ],
        lstm_service: LSTMService = Provide[
            Suite2Container.lstm_service
        ],
        nn_service: NNService = Provide[
            Suite2Container.nn_service
        ],
) -> None:
    pcapfile = Path("/Users/princio/Downloads/Day0/20160423_235403.pcap")

    nn = nn_service.get_model(1)
    model = lstm_service.load_model(nn.model_json, Path(nn.hf5_file.name))

    dn_s = dn_service.get(100)

    dn_rev, Y = lstm_service.run(model, dn_s['dn'], None)

    dn_s['rev'] = dn_rev
    dn_s['Y'] = Y

    print(dn_s)

    dn_s['Y.2'] = dn_s['Y'].round(2)
    dn_s['eps.2'] = dn_s['eps'].round(2)
    dn_s['diff.2_'] = ((dn_s['Y.2'] - dn_s['eps.2']) * 10e6).astype(int)
    mask = (dn_s['Y.2'] == dn_s['eps.2'])
    print(dn_s[~mask])
    print((dn_s['diff.2_'] == 0).all())
    pass

if __name__ == "__main__":
    application = Suite2Container()
    
    application.config.from_dict({
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
        "workdir": "/tmp/suite2_workdir"
    })

    application.wire(modules=[__name__])

    test_lstm()

    pass