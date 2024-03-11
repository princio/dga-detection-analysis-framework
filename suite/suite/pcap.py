import math
from pathlib import Path
from typing import OrderedDict, Union

import numpy as np

from malware import Malwares
from config import Config, ConfigPCAP
from subprocess_utils import subprocess_launch

import pandas as pd
import json


def parse_config_pcap(pcapfile: Path):
    configpcap = ConfigPCAP()
    configpcap.path = pcapfile
    with open(configpcap.path.parent.joinpath("pcap.json"), 'r') as fp_pcap:
        pcap = json.load(fp_pcap)
        configpcap.name = pcap['name']
        if pcap["name"] != pcapfile.stem:
            print("Warning: different names from file and json (%s <> %s)" % (configpcap.name, pcapfile.stem))
            pass
        configpcap.infected = pcap['infected']
        configpcap.dataset = pcap['dataset']
        configpcap.terminal = pcap['terminal']
        configpcap.day = pcap['day']
        configpcap.days = pcap['days']
        configpcap.sha256 = pcap['sha256']
        if configpcap.infected:
            configpcap.malware.name = pcap['malware']['name']
            configpcap.malware.year = pcap['malware']['year']
            configpcap.malware.md5 = pcap['malware']['md5']
            configpcap.malware.sha256 = pcap['malware']['sha256']
            configpcap.malware.binary = pcap['malware']['binary']
            configpcap.malware.dga = pcap['malware']['dga']
            pass
        pass
    return configpcap

class PCAP:
    def __init__(self, config: Config, malwares: Malwares, pcapfile: Path) -> None:
        self.config: Config = config
        self.configpcap: ConfigPCAP = parse_config_pcap(pcapfile)
        
        self.pcapfile = pcapfile
        self.id: Union[int,None] = None

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
        malware = self.malwares.get_or_insert(self.configpcap)
        
        qr = df.shape[0]
        q = df[df.qr == "q"].shape[0]
        r = qr - q
        u = df.dn.drop_duplicates().shape[0]
        nx_ratio = self.nx_ratio(df)

        fnreq_max = int(df.fnreq.max())

        cursor = self.config.psyconn.cursor()

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
            RETURNING id;
            """,
            (
                self.pcapfile.name, malware.id, self.configpcap.sha256, self.configpcap.infected,
                self.configpcap.dataset, self.configpcap.day, self.configpcap.days, self.configpcap.terminal,
                qr, q, r, u,
                fnreq_max, nx_ratio 
            )
        )

        lastrow = cursor.fetchone()
        if lastrow is None:
            raise Exception("Impossible to retrieve the pcap id")
        if lastrow[0] == 0:
            raise Exception("Retrieved pcap id is 0 something si wrong")
        self.id = lastrow[0]

        print("[info]: pcap successfully inserted with id %s" % self.id)

        cursor.execute("""CREATE TABLE IF NOT EXISTS public.message_%s PARTITION OF public.message FOR VALUES IN (%s);""", (self.id, self.id, ))

        cursor.close()
        cursor.connection.commit()
    pass

    def run_postgres_dn(self, df: pd.DataFrame):
        df_u = df.drop_duplicates(subset="dn").copy()

        cursor = self.config.psyconn.cursor()

        tuples = tuple( row["dn"] for _, row in df_u.iterrows() )

        values = [ [ x["dn"], x["bdn"], x["tld"], x["icann"], x["private"], not bool(x["is_valid"]) ] for _, x in df_u.iterrows() ]
        args_str = b",".join([cursor.mogrify("(%s,%s,%s,%s,%s,%s)", x) for x in values])
        cursor.execute(b"""
            INSERT INTO public.dn(
                dn, bdn, tld, icann, private, invalid)
                VALUES """ +
            args_str +
            b" ON CONFLICT DO NOTHING RETURNING id")
        
        cursor.connection.commit()

        if cursor.rowcount < df_u.shape[0]:
            cursor.execute("""SELECT id from dn WHERE dn.dn IN %s""", (tuples, ))
            pass

        df_u["dn_id"] = [t[0] for t in cursor.fetchall()]
        df = df.merge(df_u[["dn", "dn_id"]], left_on="dn", right_on="dn")

        cursor.connection.commit()
        cursor.close()

        return df

    pass


    def run_postgres_message(self, df: pd.DataFrame):

        cursor = self.config.psyconn.cursor()

        values = [
            [
                x["fn"], self.id, x["time"], x["fn"],
                x["fnreq"], x["dn_id"], x["qcode"],
                x["qr"] == "r", x["rcode"], x["server"],
                x["answer"]
            ]
            for _, x in df.iterrows()
        ]
        args_str = b",".join([cursor.mogrify("(%s, %s,%s,%s, %s,%s,%s, %s,%s,%s, %s)", x) for x in values])
        cursor.execute(f"""
            INSERT INTO public.message_{self.id}(
	            id, pcap_id, time_s, fn,
                fn_req, dn_id, qcode,
                is_r, rcode, server,
                answer)
	        VALUES""" + args_str.decode("utf-8") + 
            " ON CONFLICT DO NOTHING")
        
        cursor.connection.commit()
        cursor.close()
        pass

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

        df.tld.fillna("", inplace=True)
        df.icann.fillna("", inplace=True)
        df.private.fillna("", inplace=True)
        df.server.fillna("", inplace=True)
        df.answer.fillna("", inplace=True)

        df.index.set_names("fn", inplace=True)
        df = df.reset_index()
        
        self.run_postgres_pcap(df)

        batch_size = 2000
        batch_num = math.ceil(df.shape[0] / batch_size)
        for batch in range(0, batch_num):
            batch_start = batch_size * batch
            batch_end = batch_size * (batch + 1) if (batch + 1) < batch_num else df.shape[0]

            print("[info]: processing batch %s/%s..." % (batch + 1, batch_num))

            df_batch = df.iloc[batch_start:batch_end]

            df_batch = self.run_postgres_dn(df_batch)

            self.run_postgres_message(df_batch)

            pass

        pass