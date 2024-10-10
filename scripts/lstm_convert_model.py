import logging
from pathlib import Path
import subprocess
import sys
from tempfile import TemporaryFile
from dependency_injector.wiring import Provide, inject
from packaging.version import Version

sys.path.append(str(Path(__file__).resolve().parent.parent.joinpath('suite2').absolute()))
from suite2.psltrie.psltrie_service import PSLTrieService
from suite2.defs import NNType
from suite2.dn.dn_service import DNService
from suite2.lstm.lstm_service import LSTMService
from suite2.nn.nn_service import NNService
from suite2.pcap.pcap_service import PCAPService
from suite2.container import Suite2Container


def testnn_none(dn_service, lstm_service, model):
    df_dn = dn_service.get(1000, 1)

    dn_rev = lstm_service.reverse(df_dn['dn'].tolist())

    df_dn['Y'] =  model(dn_rev)

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
) -> None:
    nns = nn_service.get_all()

    for nn in nns:
        import tensorflow as tf
        print(tf.version.VERSION)
        if Version(tf.version.VERSION) <= Version("2.13"):
            model = tf.keras.models.load_model(nn.zipdir.name)
            model.compile()
            model.save(f'{nn.name}.tf.keras.hf5')
            import keras
            model = keras.models.load_model(nn.zipdir.name)
            model.compile()
            model.save(f'{nn.name}.keras.hf5')
        else:
            # model = keras.layers.TFSMLayer(nn.zipdir.name,
            # call_endpoint='serving_default')
            import keras
            model = keras.models.load_model(f'{nn.name}.tf.keras.hf5')
            pass
        testnn_none(dn_service, lstm_service, model)
        break
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