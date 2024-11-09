import logging
import math
from pathlib import Path
import pathlib
import subprocess
import sys
from tempfile import TemporaryFile
from dependency_injector.wiring import Provide, inject
import pandas as pd
import time

sys.path.append(str(Path(__file__).resolve().parent.parent.joinpath('suite2').absolute()))
from suite2.psltrie.psltrie_service import PSLTrieService
from suite2.defs import NNType
from suite2.dn.dn_service import DNService
from suite2.lstm.lstm_service import LSTMService
from suite2.nn.nn_service import NNService
from suite2.pcap.pcap_service import PCAPService
from suite2.container import Suite2Container

@inject
def main(
        dn_service: DNService = Provide[
            Suite2Container.dn_service
        ],
        lstm_service: LSTMService = Provide[
            Suite2Container.lstm_service
        ],
        nn_service: NNService = Provide[
            Suite2Container.nn_service
        ],
        psltrie_service: NNService = Provide[
            Suite2Container.psltrie_service
        ],
) -> None:
    nns = nn_service.get_all()

    batch = 500_000

    nbatch = math.ceil(123177254 / batch)

    nns_model = [ nn_service.get_model(nn.id) for nn in nn_service.get_all() ]
    loaded_models = [ lstm_service.load_model(nn.model_json, Path(nn.hf5_file.name)) for nn in nns_model ]

    for i in range(nbatch):
        if Path(f'./lstm_dac/dac_csv_{i}.csv').exists():
            continue

        print(f'{i + 1}/{nbatch + 1}')
        
        df = pd.read_csv('dac_dns.csv', header=None, skiprows=batch * i, nrows=batch, sep='\t', na_values='\\N', low_memory=False)
        df.columns = ['dn','year','family','tranco_rank']
        for idxnn, loaded_model in enumerate(loaded_models):
            (dn_rev, Y), suffixes = dn_service.run(df['dn'], (nns[idxnn].nntype, loaded_model))

            if nns[idxnn].name != 'none':
                df[nns[idxnn].name] = suffixes
            df[f'eps_{nns[idxnn].name}'] = Y
            pass
        # df['year'] = df['year'].astype(int)
        df['private'] = df['private'].where(df['private'] != df['icann'], None)
        df['icann'] = df['icann'].where(df['icann'] != df['tld'], None)
        df[['dn', 'year', 'family', 'tranco_rank', 'eps_none', 'eps_tld', 'eps_icann', 'eps_private', 'tld', 'icann', 'private']]\
            .to_csv(f'./lstm_dac/dac_csv_{i}.csv', index=False)
        
        time.sleep(20)
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

    main()

    application.shutdown_resources()

    pass