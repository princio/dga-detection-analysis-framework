import logging
from pathlib import Path
import re
import subprocess
import sys
from tempfile import TemporaryDirectory
from dependency_injector.wiring import Provide, inject
import pandas as pd

sys.path.append(str(Path(__file__).resolve().parent.parent.joinpath('suite2').absolute()))
from suite2.dn.dn_service import DNService
from suite2.pcap.pcap_service import PCAPService
from suite2.container import Suite2Container


ROOT = Path('/Users/princio/Downloads/IT2016/')

class Seven7Zip:
    BINARY='7zz'
    def __init__(self, archive: Path):
        self.archive = archive

        output = subprocess.getoutput(f'7zz l {self.archive}')
        pcaps = re.findall(rf'Day\d\/.*?pcap$', output, re.MULTILINE)
        if len(pcaps) < 24:
            raise Exception('Len is ' + str(len(pcaps)))
        self.pcaps = sorted(pcaps)
        pass

    def extract(self, index: int, outdir: Path):
        subprocess.run([ '7zz', 'x', '-y', self.archive, self.pcaps[index], f'-o{outdir}' ], check=True, capture_output=True)
        logging.info(f'Extraction of {self.pcaps[index]} success.')
        return outdir.joinpath(self.pcaps[index])
    
    def name(self, index: int):
        return self.pcaps[index][5:] # Removing «Dayx/»
    pass


@inject
def test_pcap(
        pcap_service: PCAPService = Provide[
            Suite2Container.pcap_service
        ],
        dn_service: DNService = Provide[
            Suite2Container.dn_service
        ],
) -> None:
    for day in range(0,1):
        s7zip = Seven7Zip(ROOT.joinpath(f'Day{day}').with_suffix('.7z'))

        pcaps = []
        for h in range(2):
            pcapname = s7zip.name(h)
            pcap_id = None
            try:
                pcap_id = pcap_service.getbyname(pcapname).id
            except Exception as e:
                logging.info(e)
                pass
        
            csvfile = pcap_service.processed_path(pcapname)
            if not (pcap_id and csvfile.exists()):
                with TemporaryDirectory('_pcaps') as tmpdir:
                    pcapfile = s7zip.extract(h, Path(tmpdir))
                    csvfile = pcap_service.process(pcapfile)
                    pcap_id = pcap_service.add(pcapfile)
                    pass
                pass
            pcaps.append((pcap_id, csvfile))
            pass
        
        dfs = []
        for pcap in pcaps:
            df = pd.read_csv(pcap[1])
            df = df.head(1000)
            df['dn_id'] = pcap_service.dn_service.add(df["dn"]) # important
            dn_service.dbfill()
            df = pcap_service.message_service.dns_parse_preprocess(df, pcap_id)
            dfs.append(df)
            pass

        partition_name = f'it2016_{day}'
        pcap_service.message_service.create_partition(partition_name, [pcap[0] for pcap in pcaps])
        df = pd.concat(dfs)
        pcap_service.message_service.copy(partition_name, df)
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
        "logging": logging.DEBUG
    })

    application.init_resources()

    application.wire(modules=[__name__])

    test_pcap()

    application.shutdown_resources()

    pass