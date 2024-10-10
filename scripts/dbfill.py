import logging
from pathlib import Path
import sys
from dependency_injector.wiring import Provide, inject

sys.path.append(str(Path(__file__).resolve().parent.parent.joinpath('suite2').absolute()))
from suite2.dn.dn_service import DNService
from suite2.container import Suite2Container

@inject
def main(
        dn_service: DNService = Provide[
            Suite2Container.dn_service
        ],
) -> None:
    dn_service.dbfill()
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
        "logging": logging.DEBUG
    })

    application.init_resources()

    application.wire(modules=[__name__])

    main()

    application.shutdown_resources()

    pass