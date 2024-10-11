
from .container import Suite2Container


if __name__ == "__main__":
    container = Suite2Container(config={
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
        "psltrie": "/Users/princio/Repo/princio/malware-detection-predict-file/psltrie/bin/binary_prod"
    })
    
    container.init_resources()

    container.shutdown_resources()