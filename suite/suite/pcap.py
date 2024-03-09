import math
from pathlib import Path

import numpy as np

from malware import Malwares
from config import Config
from subprocess_utils import subprocess_launch

import pandas as pd
import hashlib

class PCAP:
    def __init__(self, config: Config, malwares: Malwares, pcapfile: Path) -> None:
        self.config: Config = config
        self.pcapfile = pcapfile

        self.malwares = malwares
    
        self.pslregex2_path: Path = self.config.workdir.pslregex2.joinpath("etld.csv")
        self.tshark_path: Path = self.config.workdir.path.joinpath(self.pcapfile.with_suffix(".dns.pcap").name)
        self.dns_parse_path: Path = self.config.workdir.path.joinpath(self.pcapfile.with_suffix(".csv").name)

        pass

    def run_plsregex2(self):
        subprocess_launch(self.config,
                          self.pslregex2_path,
                          "pslregex2", [
                              self.config.bin.python_pslregex2,
                              self.config.pyscript.pslregex2,
                              self.config.workdir.pslregex2
                        ])
        pass


    def run_tshark(self):
        subprocess_launch(self.config,
                          self.tshark_path,
                          "tshark", [
                              self.config.bin.tshark,
                              "-r", self.pcapfile,
                              "-Y", "dns && !_ws.malformed && !icmp",
                              "-w", self.tshark_path
                        ])
        pass

    def run_dns_parse(self):
        subprocess_launch(self.config,
                          self.dns_parse_path,
                          "dns_parse", [
                              self.config.bin.dns_parse,
                              "-i", self.pslregex2_path,
                              "-o", self.dns_parse_path,
                              self.tshark_path
                        ])
        pass

    def run_postgres_pcap(self, df: pd.DataFrame):
        malware = self.malwares.get_or_insert()
        
        hash_rawpcap = hashlib.sha256(open(self.pcapfile, 'rb').read()).hexdigest()

        if hash_rawpcap != self.config.pcap.sha256:
            print("SHA256 not equal:\n   json {self.config.pcap.sha256}\ncomputed {hash_rawpcap}")
            exit(1)

        qr = df.shape[0]
        q = df[df.qr == "q"].shape[0]
        r = qr - q
        u = df.dn.drop_duplicates().shape[0]
        nx_ratio = self.nx_ratio(df)

        fnreq_max = int(df.fnreq.max())

        cursor = self.config.psyconn.cursor()

        try:
            cursor.execute(
                """
                INSERT INTO pcap (
                    name, malware_id, sha256, infected, 
                    dataset, day, days, terminal, 
                    qr, q, r, u, 
                    fnreq_max, dga_ratio
                ) VALUES (
                    %s, %s, %s, %s::boolean,
                    %s, %s, %s, %s, 
                    %s::bigint,%s::bigint,%s::bigint,%s::bigint,
                    %s::bigint, %s::real
                )
                RETURNING id
                """,
                (
                    self.pcapfile.name, malware.id, hash_rawpcap, self.config.pcap.infected,
                    self.config.pcap.dataset, self.config.pcap.day, self.config.pcap.days, self.config.pcap.terminal,
                    qr, q, r, u,
                    fnreq_max, nx_ratio 
                )
            )
        except Exception as error:
            print(error)
            pass
        pass
    pass

    def run_postgres_dn(self, df: pd.DataFrame):
        df_u = df.drop_duplicates(subset="dn").copy()

        cursor = self.config.psyconn.cursor()

        df.tld.fillna("", inplace=True)
        df.icann.fillna("", inplace=True)
        df.private.fillna("", inplace=True)

        tuples = tuple( row["dn"] for _, row in df_u.iterrows() )

        values = [ [ x["dn"], x["bdn"], x["tld"], x["icann"], x["private"], bool(x["is_valid"]) ] for _, x in df_u.iterrows() ]
        bo = [cursor.mogrify("(%s,%s,%s,%s,%s,%s)", x) for x in values]
        args_str = b",".join(bo)
        cursor.execute(b"""
            INSERT INTO public.dn(
                dn, bdn, tld, icann, private, invalid)
                VALUES """ +
            args_str +
            b" ON CONFLICT DO NOTHING RETURNING id")
        
        cursor.connection.commit()

        if cursor.rowcount < df_u.shape[0]:
            s = cursor.execute("""SELECT id from dn WHERE dn.dn IN %s""", (tuples, ))
            print(s)
            pass

        df_u["id"] = [t[0] for t in cursor.fetchall()]
        df = df.merge(df_u[["dn", "id"]], left_on="dn", right_on="dn")
        print(df.head())
        print(df.head().to_markdown())

        return df

    pass

    def nx_ratio(self, df: pd.DataFrame):
        n_nx = (df.rcode == 3).sum()
        n = df.shape[0]

        u_nx = df[df.rcode == 3].drop_duplicates(subset='dn').shape[0]
        u = df.drop_duplicates(subset='dn').shape[0]

        u_ratio = u_nx / u
        n_ratio = n_nx / n
    
        return (u_ratio + n_ratio) / 2

    def run(self):

        self.run_plsregex2()
        self.run_tshark()
        self.run_dns_parse()

        df = pd.read_csv(self.dns_parse_path, index_col=False)
        
        # self.run_postgres_pcap(df)

        batch_size = 2000
        batch_num = math.ceil(df.shape[0] / batch_size)
        for batch in range(0, batch_num):
            batch_start = batch_size * batch
            batch_end = batch_size * (batch + 1) if (batch + 1) < batch_num else df.shape[0]

            df_batch = df.iloc[batch_start:batch_end]

            df_batch = self.run_postgres_dn(df_batch)

            self.run_postgres_message(df_batch)

        pass