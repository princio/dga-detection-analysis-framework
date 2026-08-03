import logging
from pathlib import Path
import subprocess
import sys
from tempfile import NamedTemporaryFile, TemporaryFile
from dependency_injector.wiring import Provide, inject
import pandas as pd
import psycopg2


sys.path.append(str(Path(__file__).resolve().parent.parent.joinpath('suite2').absolute()))
from suite2.subprocess.subprocess_service import SubprocessService
from suite2.pcap.pcap_service import PCAPService
from suite2.container import Suite2Container


def get_line(e: pd.errors.ParserError, file):
    import re
    match = re.search(r'Error tokenizing data\. C error: Expected \d+ fields in line (\d+)', e.__str__())
    lines = []
    if match:
        line_number  = int(match.group(1))
        with open(file, 'r') as file:
            for i, line in enumerate(file):
                if i >= line_number - 2:
                    if i <= line_number + 2:
                        lines.append(line)
                    else:
                        break
    return lines

@inject
def fix_utf8(
        pcap_service: PCAPService = Provide[
            Suite2Container.pcap_service
        ],
        subprocess_service: SubprocessService = Provide[
            Suite2Container.subprocess_service
        ],
) -> None:
    pcapfile = Path('/Users/princio/Downloads/IT2016/Day/20160425_095437.pcap')

    o1 = subprocess_service.launch_tcpdump(pcapfile)
    o2 = subprocess_service.launch_tshark(o1)
    file2fixpath = subprocess_service.launch_dns_parse(o2)

    # with open(file2fixpath, 'r') as infile:
    #     for i, line in enumerate(infile):
    #         if line.find('dns_parse') >= 0:
    #             print(i, line)
    try:
        df = pd.read_csv(file2fixpath) #, encoding='utf-8')#, encoding_errors='replace')
        print(df)
    except pd.errors.ParserError as e:
        print(e)
        lines = get_line(e,  file2fixpath)
        print(''.join(lines))
        pass

    # exit(0)
    # with open(file2fixpath, 'r', encoding='utf-8', errors='ignore') as infile, \
    #     open(filefixedpath, 'w', encoding='ascii', errors='ignore') as outfile:
    #     for i, line in enumerate(infile):
    #         outfile.write(line)
    # try:
    #     df = pd.read_csv(filefixedpath)#, encoding='ascii')
    #     print(df)
    # except pd.errors.ParserError as e:
    #     print(e)
    #     line = get_line(e,  file2fixpath)
    #     print(line)
    #     pass
    # with open(file2fixpath, 'r', encoding='utf-8', errors='ignore') as infile, \
    #     open(filefixedpath, 'w', encoding='ascii', errors='ignore') as outfile:
    #     for i, line in enumerate(infile):
    #         outfile.write(line)
    
    # try:
    #     df = pd.read_csv(filefixedpath, encoding='ascii', encoding_errors='ignore')
    # except pd.errors.ParserError as e:
    #     line = get_line(e,  filefixedpath)
    #     print(line)
    
        # import re
        # match = re.search(r'Error tokenizing data\. C error: Expected \d+ fields in line (\d+)', e.__str__())
        # if match:
        #     line_number  = int(match.group(1))
        #     with open(file2fixpath, 'r') as file:
        #         lines = [ line for i, line in enumerate(file) if (i > line_number - 2 and i < line_number + 2) ]
        #         for i, line in enumerate(lines):
        #             print(i + line_number - 2, line)
        #             print(i + line_number - 2, [line[:-1].encode('ascii',errors='ignore').decode('ascii').replace('\n', '[new-line]')])
        #             print()
        #             pass
        #         pass
        #     pass

    # print('read2')
    # with open(f, 'r') as file:
    #     lines = [line[:-1].encode('ascii',errors='ignore').decode('ascii').replace('\n', '') for line in file ]
    #     pass

    # print('write2')
    # with open(f'{f}.2', 'w') as file:
    #     file.writelines(lines)
    #     pass
    # df = pd.read_csv(f'{f}.2')
    
    return


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

    fix_utf8()

    application.shutdown_resources()

    pass