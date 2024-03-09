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


class Paths:
    def __init__(self, root_outdir, pcapfile) -> None:
        self.root_outdir: str = str(Path(root_outdir).absolute())

        self.dir_pslregex2: str = os.path.join(root_outdir, "pslregex2")
        self.dir_pslregex2: str = str(Path(self.dir_pslregex2).absolute())

        self.path_pslregex2: str = os.path.join(self.dir_pslregex2, "etld.csv")
        self.path_pslregex2: str = str(Path(self.path_pslregex2).absolute())

        self.path_pcapfile: str = str(Path(pcapfile).absolute())
        self.path_malware: str = str(Path(self.path_pcapfile).parent.absolute().joinpath("malware.json"))

        self.basename = os.path.basename(self.path_pcapfile)

        self.noext = Path(self.basename).stem

        self.outdir: str = str(Path(self.root_outdir, self.noext).absolute())
        self.dir_logs: str = str(Path(self.outdir, "logs").absolute())
    
        self.name_dnspcap = f"{self.noext}.onlydns.pcap"
        self.path_tshark_output = str(Path(os.path.join(self.outdir, self.name_dnspcap)).absolute())

        self.path_dns_parse_output = str(Path(os.path.join(self.outdir, f"{self.noext}.csv")).absolute())
        self.path_lstm_output = str(Path(os.path.join(self.outdir, f"{self.noext}.lstm.csv")).absolute())

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

            self.postgre = conf['postgre']
                
            self.tshark: str = conf['bin']['tshark']

            self.python: str = conf['bin']['python']

            self.python_pslregex2: str = conf['bin']['python_lstm']

            self.python_lstm: str = conf['bin']['python_lstm']

            self.dns_parse: str = conf['bin']['dns_parse']

            self.script_pslregex2: str = conf['pyscript']['pslregex2']

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
                    print(f"{error}TShark not installed")
                    error = True
                pass

            error = error or self.check_python(self.python, fplog)
            error = error or self.check_python(self.python_pslregex2, fplog)
            error = error or self.check_python(self.python_lstm, fplog)

            if not os.path.exists(self.script_pslregex2):
                print(f"{error}PSLRegex script not exists: {self.script_pslregex2}")
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
            with open(os.path.join(paths.dir_logs, f"{name}.log"), "w") as fplog:
                p = subprocess.Popen(
                    args,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    text=True
                )

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

    paths = Paths(args.root_outdir, args.pcapfile)
    binaries = Binaries(paths)

    paths.check()
    binaries.test()
    print("[info]: binaries checked")



    # pslregex2

    binaries.launch(paths.path_pslregex2, "pslregex2", [
        binaries.python_pslregex2,
        binaries.script_pslregex2,
        paths.dir_pslregex2
    ])



    # tshark

    binaries.launch(paths.path_tshark_output, "tshark", [
        binaries.tshark,
        "-r", paths.path_pcapfile,
        "-Y", "dns && !_ws.malformed && !icmp",
        "-w", paths.path_tshark_output
    ])



    # dns_parse

    binaries.launch(paths.path_dns_parse_output, "dns_parse", [
        binaries.dns_parse,
        "-i", paths.path_pslregex2,
        "-o", paths.path_dns_parse_output,
        paths.path_tshark_output
    ])



    # lstm

    binaries.launch(paths.path_lstm_output, "lstm", [
        binaries.python_lstm,
        binaries.script_pslregex2,
        binaries.nndir,
        paths.outdir,
        paths.path_dns_parse_output
    ])



    # postgres

    import pandas as pd
    from sqlalchemy import create_engine
    from sqlalchemy.engine import URL
    import psycopg2 as psy

    psyconn = psy.connect("host=%s dbname=%s user=%s password=%s" % (binaries.postgre['host'], binaries.postgre['user'], binaries.postgre['dbname'], binaries.postgre['password']))

    url = URL.create(
        drivername="postgresql",
        host=binaries.postgre['host'],
        username=binaries.postgre['user'],
        database=binaries.postgre['dbname'],
        password=binaries.postgre['password']
    )
    engine = create_engine(url)
    connection = engine.connect()

    df = pd.read_csv(paths.path_lstm_output)



    # malware
    # malware
    # malware

    malware_notinfected_id = 28
    malware_id = 28

    with open(paths.path_malware, 'r') as fp:
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

    hash_rawpcap = hashlib.sha256(open(paths.path_pcapfile,'rb').read()).hexdigest()

    qr = df.shape[0]
    q = df[df.qr == "q"].shape[0]
    r = qr - q
    u = df.dn.drop_duplicates().shape[0]
    cursor.execute(
        """
        INSERT INTO pcap
            (name, hash, infected, qr, q, r, "unique", unique_norm, dga_ratio)
        VALUES
            (%s,%s,%s::boolean,%s::bigint,%s::bigint,%s::bigint,%s::bigint,%s::real, %s::integer)
        RETURNING id
        """,
        [paths.basename, hash_rawpcap, malware_id != 28, qr, q, r, u, u / qr, 1],
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