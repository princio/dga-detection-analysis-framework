import logging
import os
from pathlib import Path
import subprocess
from typing import Dict


from ..utils import mkdir27

class SubprocessService:
    INPUT_FLAG = f'%input'
    OUTPUT_FLAG = f'%output'
    BINARY_NAMES=['tcpdump', 'tshark', 'dns_parse', 'psltrie', 'iconv']
    def __init__(self, binaries: Dict[str,Path], workdir: Path) -> None:

        self.binaries = binaries

        if not all([ b in binaries for b in SubprocessService.BINARY_NAMES]):
            raise Exception('Some binary is not defined.')

        self.workdir = mkdir27(Path(workdir))
        for b in SubprocessService.BINARY_NAMES:
            mkdir27(Path(workdir).joinpath(b))
            pass
        return
    
    def clean(self, input: Path):
        for bin in SubprocessService.BINARY_NAMES:
            files = self.workdir.joinpath(bin).glob(f'{input.name}*')
            for file in files:
                logging.getLogger(__name__).debug(f'Removing file {file}.')
                os.remove(file)
                pass
            pass
        logging.getLogger(__name__).info(f'Cleaned file {input}.')
        pass

    def launch(self, name, input: Path, args, output_is_stdout=False):
        outdir = self.workdir.joinpath(name)
        output = outdir.joinpath(f'{input.name}.{name}')
        logfile = outdir.joinpath(f'{input.name}.{name}.log')
        errlogfile = outdir.joinpath(f'{input.name}.{name}.err')

        if output.exists():
            logging.getLogger(__name__).debug(f"{name} already done at «{output}» for input: «{input}»")
            return output
        
        logging.getLogger(__name__).debug(f"Proceeding with {name} with input: «{input}» and output «{output}»")
        
        args = [ input if arg == SubprocessService.INPUT_FLAG else arg for arg in args ]
        args = [ output if arg == SubprocessService.OUTPUT_FLAG else arg for arg in args ]
        args = [ arg.absolute() if isinstance(arg, Path) else arg for arg in args ]
        args = [ self.binaries[name] ] + args

        try:
            p = subprocess.Popen(
                args,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
        except Exception as e:
            logging.getLogger(__name__).error("Error with arguments:\n> %s" % "\n> ".join([ str(arg) for arg in args ]))
            raise Exception('Subprocess failure.') from e

        stdout, stderr = p.communicate(timeout=100000)

        with open(output if output_is_stdout else logfile, 'w') as fp:
            fp.writelines(stdout)
            pass

        with open(errlogfile, "w") as logerr:
            logerr.writelines(stderr)
            pass

        if p.returncode != 0 and output is not None and output.exists():
            logging.getLogger(__name__).debug(f"Renaming output file {output} with suffix «.error».")
            output.rename(f"{output}.error")

        if p.returncode != 0:
            logerror = '\n~'.join(stderr.split('\n'))
            raise Exception(f"{name} execution failed:\n{logerror}")
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
        time,size,protocol,src,dst,qr,AA,rcode,fnreq,qdcount,ancount,nscount,arcount,qcode,dn,answer

        Args:
            pcapfile (Path): Input .pcap file.

        Returns:
            Path: The output file path.
        """
        output = self.launch(
            "dns_parse",
            pcapfile, [
                "-o", SubprocessService.OUTPUT_FLAG,
                SubprocessService.INPUT_FLAG
            ])
        return output

    def launch_iconv(self, input: Path) -> Path:
        """Return the dataframe output of dns_parse having the following
        columns:
        time,size,protocol,server,qr,AA,rcode,fnreq,qdcount,ancount,nscount,arcount,qcode,dn,answer

        Args:
            pcapfile (Path): Input .pcap file.

        Returns:
            Path: The output file path.
        """
        output = self.launch(
            "iconv",
            input, [
                "-f", "utf-8",
                "-t", "utf-8",
                "-c",
                SubprocessService.INPUT_FLAG,
            ], output_is_stdout=True)
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