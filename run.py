import json
import subprocess
import os, argparse
from pathlib import Path
import shutil

def rename_file_if_error(test: bool, path_oldfile: str):
    if test:
        path_errorfile = f"{path_oldfile}.error"
        if os.path.exists(path_oldfile):
            if os.path.exists(path_oldfile):
                os.remove(path_errorfile)
                pass
            os.rename(path_oldfile, path_errorfile)
            pass
        pass

class Paths:
    def __init__(self, root_outdir, pcapfile) -> None:
        self.root_outdir: str = str(Path(root_outdir).absolute())

        self.dir_pslregex2: str = os.path.join(root_outdir, "pslregex2")
        self.dir_pslregex2: str = str(Path(self.dir_pslregex2).absolute())

        self.path_pslregex2: str = os.path.join(self.dir_pslregex2, "etld.csv")
        self.path_pslregex2: str = str(Path(self.path_pslregex2).absolute())

        self.path_pcapfile: str = str(Path(pcapfile).absolute())

        self.basename = os.path.basename(self.path_pcapfile)

        self.noext = Path(self.basename).stem

        self.outdir: str = str(Path(self.root_outdir, self.noext).absolute())
        self.dir_logs: str = str(Path(self.outdir, "logs").absolute())
    
        self.name_dnspcap = f"{self.noext}.onlydns.pcap"
        self.path_dnspcap = str(Path(os.path.join(self.outdir, self.name_dnspcap)).absolute())

        self.path_csvfile = str(Path(os.path.join(self.outdir, f"{self.noext}.csv")).absolute())

        pass

    def check(self):
        if not os.path.exists(paths.path_pcapfile):
            raise FileNotFoundError("PCAP file not exists.")
            pass
        if not os.path.exists(paths.outdir):
            print(f"[info]: making output directory: {paths.outdir}")
            os.makedirs(paths.outdir, exist_ok=True)
            pass

        if not os.path.exists(paths.dir_logs):
            print(f"[info]: making logs directory: {paths.dir_logs}")
            os.mkdir(paths.dir_logs)
            pass
        else:
            shutil.rmtree(paths.dir_logs, ignore_errors=True)
            os.mkdir(paths.dir_logs)
            print(f"[info]: logs directory cleaned.")
            pass
        pass 

        if not os.path.exists(paths.dir_pslregex2):
            print(f"[info]: making pslregex2 directory: {paths.dir_pslregex2}")
            os.mkdir(paths.dir_pslregex2)
            pass
        pass
    pass


class Binaries:
    def __init__(self, paths) -> None:
        with open("conf.json", 'r') as fp_conf:
            conf = json.load(fp_conf)

            if 'tshark' not in conf:
                raise Exception("No tshark field")
            self.tshark: str = conf['tshark']

            if 'python' not in conf:
                raise Exception("No python field")
            self.python: str = conf['python']

            if 'pslregex2' not in conf:
                raise Exception("No pslregex2 field")
            self.pslregex2: str = conf['pslregex2']

            if 'dns_parse' not in conf:
                raise Exception("No dns_parse field")
            self.dns_parse = conf['dns_parse']

            self.paths: Paths = paths
        pass

    def test(self):
        error = ""
        with open(os.path.join(self.paths.dir_logs, "binaries.log"), "w") as fplog:
            if True:
                p = subprocess.Popen(
                    [self.tshark, "-v"],
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    text=True
                    )
                output, errors = p.communicate(timeout=1)
                fplog.write("# TShark\n\n")
                fplog.writelines(output)
                fplog.writelines(errors)
                if p.returncode != 0:
                    error = f"{error}TShark not installed\n"
                    pass
            if True:
                p = subprocess.Popen(
                    [self.python, "-V"],
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    text=True
                    )
                output, errors = p.communicate(timeout=1)
                fplog.write("# Python\n\n")
                fplog.writelines(output)
                fplog.writelines(errors)
                if p.returncode != 0:
                    error = f"{error}Python not installed\n"
                    pass
            if not os.path.exists(self.pslregex2):
                error = f"{error}PSLRegex script not exists: {self.pslregex2}\n"
                pass
            if not os.path.exists(self.dns_parse):
                error = f"{error}DNS Parse binary not exists: {self.dns_parse}\n"
                pass

            if len(error) > 0:
                raise Exception(error)
            pass
        pass


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
                    prog='DNSMalwareDetection',
                    description='Malware detection')
    
    parser.add_argument('root_outdir')
    parser.add_argument('pcapfile')

    args = parser.parse_args()


    paths = Paths(args.root_outdir, args.pcapfile)
    binaries = Binaries(paths)

    paths.check()
    binaries.test()
    print("[info]: binaries checked")



    # pslregex2

    if not os.path.exists(paths.path_pslregex2):
        print("[info]: pslregex2 step...")
        with open(os.path.join(paths.dir_logs, "pslregex2.log"), "w") as fplog:
            p = subprocess.Popen(
                [
                    binaries.python,
                    binaries.pslregex2,
                    paths.dir_pslregex2
                ],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )

            output, errors = p.communicate(timeout=600)

            fplog.writelines(output)
            fplog.writelines(errors)

            rename_file_if_error(p.returncode != 0, paths.path_pslregex2)

            if p.returncode != 0:
                raise Exception("pslregex2 execution failed")

            pass
        pass
    else:
        print("[info]: skipping pslregex2, file already exists.")
        pass



    # tshark

    if not os.path.exists(paths.path_dnspcap):
        print("[info]: tshark step...")
        with open(os.path.join(paths.dir_logs, "tshark.log"), "w") as fplog:
            p = subprocess.Popen(
                [
                    binaries.tshark,
                    "-r", paths.path_pcapfile,
                    "-Y", "dns && !_ws.malformed && !icmp",
                    "-w", paths.path_dnspcap
                ],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )

            output, errors = p.communicate()

            fplog.writelines(output)
            fplog.writelines(errors)

            rename_file_if_error(p.returncode != 0, paths.path_dnspcap)

            if p.returncode != 0:
                print(errors)
                raise Exception("tshark execution failed")

            pass
        pass
    else:
        print("[info]: skipping tshark, file already exists.")



    # dns_parse

    if not os.path.exists(paths.path_csvfile):
        print("[info]: dns_parse step...")
        with open(os.path.join(paths.dir_logs, "dns_parse.log"), "w") as fplog:
            p = subprocess.Popen(
                [
                    binaries.dns_parse,
                    "-i", paths.path_pslregex2,
                    "-o", paths.path_csvfile,
                    paths.path_dnspcap
                ],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )

            output, errors = p.communicate(timeout=100)

            fplog.writelines(output)
            fplog.writelines(errors)

            rename_file_if_error(p.returncode != 0, paths.path_csvfile)

            if p.returncode != 0:
                print(errors)
                raise Exception("dns_parse execution failed")

            pass
        pass
    else:
        print("[info]: skipping dns_parse, file already exists.")

    pass
