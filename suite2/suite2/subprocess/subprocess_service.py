from dataclasses import dataclass
from logging import Logger
import logging
from pathlib import Path
import subprocess
from typing import Dict

import pandas as pd

from ..utils import mkdir27

class SubprocessService:
    INPUT_FLAG = f'%input'
    OUTPUT_FLAG = f'%output'
    BINARY_NAMES=['tcpdump', 'tshark', 'dns_parse', 'psltrie']
    def __init__(self, binaries: Dict[str,Path], workdir: Path) -> None:

        self.name = 'dns_parse'
        self.binaries = binaries

        if not all([ b in binaries for b in SubprocessService.BINARY_NAMES]):
            raise Exception('Some binary is not defined.')

        self.workdir = mkdir27(Path(workdir))
        for b in SubprocessService.BINARY_NAMES:
            mkdir27(Path(workdir).joinpath(b))
            pass
        return

    def launch(self, name, input: Path, args):
        outdir = self.workdir.joinpath(name)
        output = outdir.joinpath(f'{input.name}.{name}')
        logfile = outdir.joinpath(f'{input.name}.log')

        if output.exists():
            logging.debug(f"{name} already done at «{output}» for input: «{input}»")
            return output
        
        logging.debug(f"Proceeding with {name} with input: «{input}» and output «{output}»")
        
        args = [ input if arg == SubprocessService.INPUT_FLAG else arg for arg in args ]
        args = [ output if arg == SubprocessService.OUTPUT_FLAG else arg for arg in args ]
        args = [ arg.absolute() if isinstance(arg, Path) else arg for arg in args ]
        args = [ self.binaries[name] ] + args

        with open(logfile, "w") as fplog:
            try:
                p = subprocess.Popen(
                    args,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    text=True
                )
            except Exception as e:
                logging.error("Error with arguments:\n> %s" % "\n> ".join([ str(arg) for arg in args ]))
                raise Exception('Subprocess failure.') from e

            stdout, stderr = p.communicate(timeout=100000)

            fplog.writelines(stdout)
            fplog.writelines(stderr)


            if p.returncode != 0 and output is not None and output.exists():
                logging.debug(f"Renaming output file {output} with suffix «.error».")
                output.rename(f"{output}.error")

            if p.returncode != 0:
                logoutput = '\n> '.join(stdout.split('\n'))
                logerror = '\n~ '.join(stderr.split('\n'))
                raise Exception(f"{name} execution failed:\n{logoutput}\n{logerror}")
            pass
        return output


    def launch_tcpdump(self, pcapfile: Path):
        output = self.launch(
            'tcpdump',
            pcapfile, [
                "-r", SubprocessService.INPUT_FLAG,
                "-w", SubprocessService.OUTPUT_FLAG,
                "port", "53"
            ])
        return output

    def launch_tshark(self, pcapfile: Path):
        output = self.launch(
            'tshark',
            pcapfile, [
                "-r", SubprocessService.INPUT_FLAG,
                "-Y", "dns && !_ws.malformed && !icmp",
                "-w", SubprocessService.OUTPUT_FLAG
            ])
        return output

    def launch_dns_parse(self, pcapfile: Path) -> Path:
        """Return the dataframe output of dns_parse having the following
        columns:
        time,size,protocol,server,qr,AA,rcode,fnreq,qdcount,ancount,nscount,arcount,qcode,dn,answer

        Args:
            pcapfile (Path): Input .pcap file.

        Returns:
            Path: The output file path.
        """
        output = self.launch(
            'dns_parse',
            pcapfile, [
                "-o", SubprocessService.OUTPUT_FLAG,
                SubprocessService.INPUT_FLAG
            ])
        return output
    

    def launch_psltrie(self, inputfile: Path, psllistfile: Path) -> Path:
        """Return the dataframe output of prsltrie.

        Args:
            input (pd.Series): A pandas Series containing a list of domain names.

        Returns:
            pd.DataFrame: A dataframe with the following columns:
            dn, bdn, rcode, tld, icann, private, dn_tld, dn_icann, dn_private.
            Missing suffixes has np.NaN value.
        """
        output = self.launch(
            "psltrie",
            inputfile, [
                psllistfile,
                "csv",
                "0",
                SubprocessService.INPUT_FLAG,
                SubprocessService.OUTPUT_FLAG
            ])
        
        return output
    pass