"""Main module."""

from dependency_injector.wiring import Provide, inject
import sys
from pathlib import Path
sys.path.append(str(Path(__file__).parent.parent.absolute()))
from suite2.pcap.pcap_service import PCAPService
from suite2.container import Suite2Container


@inject
def main(
        pcap_service: PCAPService = Provide[
            Suite2Container.pcap_service
        ],
) -> None:
    pass

if __name__ == "__main__":
    application = Suite2Container()
    
    application.config.from_dict({
        "db": {
            "host": "localhost",
            "user": "postgres",
            "password": "postgre",
            "db_name": "dns_mac",
            "port": 5432
        },
        "tshark": "/opt/homebrew/bin/tshark",
        "tcpdump": "/usr/sbin/tcpdump",
        "dns_parse": "/Users/princio/Repo/princio/malware-detection-predict-file/dns_parse/bin/dns_parse",
        "psltrie": "/Users/princio/Repo/princio/malware-detection-predict-file/psltrie/bin/binary_prod",
        "workdir": "/tmp/suite2_workdir"
    })

    application.wire(modules=[__name__])

    main()