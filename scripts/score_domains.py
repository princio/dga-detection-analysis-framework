"""
Batch-score a CSV of domain names with all four LSTM sub-models.

Replaces three near-identical scripts (lstm_dac.py, lstm_datasettraining.py,
tranco.py) that differed only in input path, column names, output path and
batch size. Those differences are now the DATASETS table below.

Usage:

    python scripts/score_domains.py dgarchive
    python scripts/score_domains.py training
    python scripts/score_domains.py tranco

Each batch is written as a separate CSV and skipped if it already exists, so a
run that dies partway through resumes where it stopped.
"""

import argparse
import logging
import math
from pathlib import Path
import sys
import time

from dependency_injector.wiring import Provide, inject
import pandas as pd

sys.path.append(str(Path(__file__).resolve().parent.parent.joinpath('suite2').absolute()))
from suite2.dn.dn_service import DNService
from suite2.lstm.lstm_service import LSTMService
from suite2.nn.nn_service import NNService
from suite2.container import Suite2Container


# The three datasets that were scored. `read_csv` is passed straight through to
# pandas; `keep` is the column order written out, before the four eps_* columns
# and the three suffix columns are appended.
DATASETS = {
    'dgarchive': {
        'source': 'dac_dns.csv',
        'read_csv': dict(header=None, sep='\t', na_values='\\N', low_memory=False),
        'columns': ['dn', 'year', 'family', 'tranco_rank'],
        'keep': ['dn', 'year', 'family', 'tranco_rank'],
        'outdir': './lstm_dac',
        'outname': 'dac_csv_{i}.csv',
        'batch': 500_000,
        'rows': 123_177_254,
    },
    'training': {
        'source': '../asset/ml/dataset_training.csv',
        'read_csv': dict(index_col=None),
        'columns': None,                        # the file has a header
        'keep': ['legit', 'class', 'dn'],
        'outdir': './lstm/training',
        'outname': 'training_{i}.csv',
        'batch': 1_000_000,
        'rows': 674_898,
    },
    'tranco': {
        'source': '../tranco/tranco_1m.csv',
        'read_csv': dict(),
        'columns': ['id', 'whitelist_id', 'dn', 'rank'],
        'keep': ['id', 'whitelist_id', 'dn', 'rank'],
        'outdir': './lstm/tranco',
        'outname': 'tranco_{i}.csv',
        'batch': 10_000_000,
        'rows': 1_000_000,
    },
}

# Seconds to idle between batches, to let the GPU/memory settle on long runs.
COOLDOWN = 20


@inject
def main(
        dataset: str,
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
    spec = DATASETS[dataset]

    outdir = Path(spec['outdir'])
    outdir.mkdir(parents=True, exist_ok=True)

    nns = nn_service.get_all()
    nns_model = [ nn_service.get_model(nn.id) for nn in nns ]
    loaded_models = [ lstm_service.load_model(nn.model_json, Path(nn.hf5_file.name)) for nn in nns_model ]

    batch = spec['batch']
    nbatch = math.ceil(spec['rows'] / batch)

    for i in range(nbatch):
        outfile = outdir.joinpath(spec['outname'].format(i=i))
        if outfile.exists():
            continue

        print(f'{i + 1}/{nbatch}')

        df = pd.read_csv(spec['source'], skiprows=batch * i, nrows=batch, **spec['read_csv'])
        if spec['columns'] is not None:
            df.columns = spec['columns']
            pass

        for idxnn, loaded_model in enumerate(loaded_models):
            (dn_rev, Y), suffixes = dn_service.run(df['dn'], (nns[idxnn].nntype, loaded_model))

            if nns[idxnn].name != 'none':
                df[nns[idxnn].name] = suffixes
                pass
            df[f'eps_{nns[idxnn].name}'] = Y
            pass

        # A suffix that equals the next-broader one carries no extra information.
        df['private'] = df['private'].where(df['private'] != df['icann'], None)
        df['icann'] = df['icann'].where(df['icann'] != df['tld'], None)

        columns = spec['keep'] + \
            ['eps_none', 'eps_tld', 'eps_icann', 'eps_private', 'tld', 'icann', 'private']
        df[columns].to_csv(outfile, index=False)

        time.sleep(COOLDOWN)
        pass
    pass


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('dataset', choices=sorted(DATASETS.keys()))
    args = parser.parse_args()

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
            "dns_parse": "../dns_parse/bin/dns_parse",
            "psltrie": "../psltrie/bin/binary_prod",
            "iconv": "/usr/bin/iconv"
        },
        "workdir": "/tmp/suite2_workdir",
        "logging": logging.INFO
    })

    application.init_resources()

    application.wire(modules=[__name__])

    main(args.dataset)

    application.shutdown_resources()

    pass
