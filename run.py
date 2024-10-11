from dataclasses import dataclass
import json
import subprocess
import os, argparse
from pathlib import Path
import shutil

def rename_file_if_error(test: bool, path_currentfile: str):
    if test:
        path_errorfile = f"{path_currentfile}.error"
        if os.path.exists(path_currentfile):
            os.rename(path_currentfile, path_errorfile)
            pass
        pass
    pass


@dataclass
class PathsDir:
    root: Path
    pcap: Path
    psl_list: Path
    log: Path
    pass

@dataclass
class PathsInput:
    pcap: Path
    json: Path
    pass

@dataclass
class PathsOutput:
    psl_list: Path
    tshark: Path
    dns_parse: Path
    lstm: Path
    pass


class Paths:
    def __init__(self, root_outdir: Path, pcapfile: Path) -> None:
        self.input = PathsInput(
            pcapfile,
            pcapfile.parent.joinpath("pcap.json")
        )

        self.dir = PathsDir(
            root_outdir,
            root_outdir.joinpath(pcapfile.stem),
            root_outdir.joinpath("psl_list"),
            root_outdir.joinpath("logs").absolute()
        )

        self.output = PathsOutput(
            self.dir.psl_list.joinpath("psl_list.csv"),
            root_outdir.joinpath(self.input.pcap.with_suffix(".onlydns.pcap").name),
            root_outdir.joinpath(self.input.pcap.with_suffix(".csv").name),
            root_outdir.joinpath(self.input.pcap.with_suffix(".lstm.csv").name)
        )

        pass

    def check(self):
        if not os.path.exists(paths.input.pcap):
            raise FileNotFoundError("PCAP file not exists.")
            pass
        if not os.path.exists(paths.dir.root):
            print(f"[info]: making output directory: {paths.dir.root}")
            os.makedirs(paths.dir.root, exist_ok=True)
            pass

        if not os.path.exists(paths.dir.log):
            print(f"[info]: making logs directory: {paths.dir.log}")
            os.mkdir(paths.dir.log)
            pass
        else:
            shutil.rmtree(paths.dir.log, ignore_errors=True)
            os.mkdir(paths.dir.log)
            print(f"[info]: logs directory cleaned.")
            pass
        pass 

        if not os.path.exists(paths.dir.psl_list):
            print(f"[info]: making psl_list directory: {paths.dir.psl_list}")
            os.mkdir(paths.dir.psl_list)
            pass
        pass
    pass


class Binaries:
    def __init__(self, paths) -> None:
        with open("conf.json", 'r') as fp_conf:
            conf = json.load(fp_conf)

            self.postgre = conf['postgre']
                
            self.tshark: str = conf['bin']['tshark']

            self.python: str = conf['bin']['python']

            self.python_psl_list: str = conf['bin']['python_psl_list']

            self.python_lstm: str = conf['bin']['python_lstm']

            self.dns_parse: str = conf['bin']['dns_parse']

            self.script_psl_list: str = conf['pyscript']['psl_list']

            self.script_lstm: str = conf['pyscript']['lstm']

            self.nndir: str = conf['nndir']

            self.paths: Paths = paths
        pass

    def check_python(self, python_path, fplog):
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
            print(f"Python not installed at: {python_path}")
        return p.returncode != 0

    def test(self):
        error = False
        with open(os.path.join(self.paths.dir.log, "binaries.log"), "w") as fplog:
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
                    print(f"{error}TShark not installed")
                    error = True
                pass

            error = error or self.check_python(self.python, fplog)
            error = error or self.check_python(self.python_psl_list, fplog)
            error = error or self.check_python(self.python_lstm, fplog)

            if not os.path.exists(self.script_psl_list):
                print(f"{error}PSLRegex script not exists: {self.script_psl_list}")
                pass
            if not os.path.exists(self.dns_parse):
                print(f"{error}DNS Parse binary not exists: {self.dns_parse}")
                pass
            if not os.path.exists(self.nndir):
                print(f"{error}NNs dir not exists: {self.nndir}")
                pass

            if error:
                raise Exception()
            pass
        pass

    def launch(self, file_output, name, args):
        if not os.path.exists(file_output):
            print(f"[info]: {name} step...")
            with open(os.path.join(paths.dir.log, f"{name}.log"), "w") as fplog:
                p = subprocess.Popen(
                    args,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    text=True
                )

                print(" ".join([str(a) for a in list(p.args)]))

                output, errors = p.communicate(timeout=100)

                fplog.writelines(output)
                fplog.writelines(errors)

                rename_file_if_error(p.returncode != 0, file_output)

                if p.returncode != 0:
                    print(output)
                    print(errors)
                    raise Exception(f"{name} execution failed")

                pass
            pass
        else:
            print(f"[info]: skipping {name}, file already exists.")
            pass
        pass


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
                    prog='DNSMalwareDetection',
                    description='Malware detection')
    
    parser.add_argument('root_outdir')
    parser.add_argument('pcapfile')

    args = parser.parse_args()


    paths = Paths(Path(args.root_outdir), Path(args.pcapfile))
    binaries = Binaries(paths)

    paths.check()
    binaries.test()
    print("[info]: binaries checked")



    # psl_list

    binaries.launch(paths.output.psl_list, "psl_list", [
        binaries.python_psl_list,
        binaries.script_psl_list,
        "-w", paths.dir.psl_list,
        paths.output.psl_list
    ])

    # tshark

    binaries.launch(paths.output.tshark, "tshark", [
        binaries.tshark,
        "-r", paths.input.pcap,
        "-Y", "dns && !_ws.malformed && !icmp",
        "-w", paths.output.tshark
    ])

    # dns_parse

    binaries.launch(paths.output.dns_parse, "dns_parse", [
        binaries.dns_parse,
        # "-i", paths.output.psl_list,
        "-o", paths.output.dns_parse,
        paths.output.tshark
    ])

    # lstm

    binaries.launch(paths.output.lstm, "lstm", [
        binaries.python_lstm,
        binaries.script_lstm,
        binaries.nndir,
        paths.dir.root,
        paths.output.dns_parse
    ])

    # postgres

    import pandas as pd
    from sqlalchemy import create_engine
    from sqlalchemy.engine import URL
    import psycopg2 as psy

    psyconn = psy.connect("host=%s dbname=%s user=%s password=%s" % (binaries.postgre['host'], binaries.postgre['dbname'], binaries.postgre['user'], binaries.postgre['password']))

    url = URL.create(
        drivername="postgresql",
        host=binaries.postgre['host'],
        username=binaries.postgre['user'],
        database=binaries.postgre['dbname'],
        password=binaries.postgre['password']
    )
    engine = create_engine(url)
    connection = engine.connect()

    df = pd.read_csv(paths.output.lstm)

    # malware
    # malware
    # malware

    malware_notinfected_id = 28
    malware_id = 28

    with open(paths.input.json, 'r') as fp:
        malware = json.load(fp)
        if not "name" in malware:
            raise Exception("Name field missing in malware.json")
            pass
        if malware['name'] == "no-infection":
            malware_id = 28
            pass
        if not "sha256" in malware:
            malware["sha256"] = ""
            pass
        if not "family" in malware:
            malware["family"] = ""
            pass
    
        print("%10s:\t%s" % ("name", malware['name']))
        print("%10s:\t%s" % ("family", malware['family']))
        print("%10s:\t%s" % ("sha256", malware['sha256']))
        pass

    if malware_id != malware_notinfected_id:
        n_nx = (df.rcode == 3).sum()
        n = df.shape[0]

        df_uniques = df.drop_duplicates(subset='dn')

        df['nx'] = df.rcode == 3
        u_nx = df[['dn', 'nx']].groupby("dn").sum()
        u = df_uniques.shape[0]

        u_ratio = u_nx / u
        n_ratio = n_nx / n
        nx_ratio = (u_ratio + n_ratio) / 2
        pass

    df_mw = pd.read_sql("SELECT * FROM malware", connection)

    if malware['sha256'] != "" and malware['sha256'] in set(df_mw['sha256']):
        cursor = psyconn.cursor()
        cursor.execute(
            """INSERT INTO public.malware(
                id, name, sha256, dga, family)
                VALUES (NULL, ?, ?, ?, ?, ?);""",
            [malware["name"], malware["sha256"], malware["dga"]],
        )
        malware_id = cursor.lastrowid
        cursor.close()
        pass

    # malware - end
    # malware - end
    # malware - end

    #######################
    
    cursor = psyconn.cursor()

    import hashlib

    hash_rawpcap = hashlib.sha256(open(paths.input.pcap,'rb').read()).hexdigest()

    qr = df.shape[0]
    q = df[df.qr == "q"].shape[0]
    r = qr - q
    u = df.dn.drop_duplicates().shape[0]
    cursor.execute(
        """
        INSERT INTO pcap
            (name, sha256, infected, qr, q, r, u, dga_ratio)
        VALUES
            (%s,%s,%s::boolean,%s::bigint,%s::bigint,%s::bigint,%s::bigint,%s::real, %s::integer)
        RETURNING id
        """,
        [paths.input.pcap.stem, hash_rawpcap, malware_id != 28, qr, q, r, u, u / qr, 1],
    )

    dn_unique = df.drop_duplicates(subset='dn')

    for _, row in dn_unique.iterrows():
        cursor.execute("""SELECT * FROM dn WHERE dn = %s""", [row["dn"]])

        dbrow = cursor.fetchone()
        if not dbrow:
            cursor.execute(
                """
                INSERT INTO public.dn(
                    id, dn, bdn, top10m, tld, icann, private, invalid, dyndns)
                    id, dn, bdn, top10m, tld, icann, private, invalid, dyndns)
                VALUES (NULL, %s, %s, %s, %s, %s, %s, %s, %s);
                """, [row["dn"], row["bdn"], row[""], row["tld"], row["icann"], row["private"]])



    pass