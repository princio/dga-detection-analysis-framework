from dataclasses import dataclass, asdict
import math
from pathlib import Path
from typing import OrderedDict, Union

import numpy as np
from psycopg2.extras import Json

from malware import Malwares
from config import Config, ConfigPCAP
from subprocess_utils import subprocess_launch

import pandas as pd
import json


@dataclass
class DBPCAPBatches:
    done: int
    size: int
    total: int
    def complete(self):
        return self.done == self.total
    pass
@dataclass
class DBPCAP:
    id: int
    name: str
    malware_id: int
    sha256: str
    infected: int
    dataset: str
    day: int
    days: int
    terminal: str
    qr: int
    q: int
    r: int
    u: int
    fnreq_max: int
    nx_ratio: float
    batches: Union[DBPCAPBatches,None]


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
        self.dbpcap: Union[DBPCAP,None] = None
        
        self.pcapfile = pcapfile

        self.malwares = malwares
    
        self.psl_list_path: Path = self.config.workdir.psl_list.joinpath("psl_list.csv")
        self.tshark_path: Path = self.config.workdir.path.joinpath(self.pcapfile.with_suffix(".dns.pcap").name)
        self.dns_parse_path: Path = self.config.workdir.path.joinpath(self.pcapfile.with_suffix(".csv").name)

        self.qr = None

        pass

    def run_psl_list(self):
        subprocess_launch(self.config,
                          self.psl_list_path,
                          "psl_list", [
                              self.config.bin.python_psl_list,
                              self.config.pyscript.psl_list,
                              "-w", self.config.workdir.psl_list,
                              self.psl_list_path
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
                              "-i", self.psl_list_path,
                              "-o", self.dns_parse_path,
                              self.tshark_path
                        ])
        pass

    def run_fetch(self):
        with self.config.psyconn.cursor() as cursor:
            cursor.execute("""SELECT * FROM pcap WHERE sha256 = %s""", (self.configpcap.sha256,))

            if cursor.rowcount == 0:
                return
        
            pcap = cursor.fetchall()[0]

            dbpcapbatches = None
            if not pcap[15] is None:
                dbpcapbatches = DBPCAPBatches(pcap[15]['done'], pcap[15]['size'], pcap[15]['total'])

            self.dbpcap = DBPCAP(
                pcap[0],
                pcap[1],
                pcap[2],
                pcap[3],
                pcap[4],
                pcap[5],
                pcap[6],
                pcap[7],
                pcap[8],
                pcap[9],
                pcap[10],
                pcap[11],
                pcap[12],
                pcap[13],
                pcap[14],
                dbpcapbatches
            )

        pass

    def run_insert(self, df: pd.DataFrame):
        malware = self.malwares.get_or_insert(self.configpcap)
        
        qr = df.shape[0]
        q = df[df.qr == "q"].shape[0]
        r = qr - q
        u = df.dn.drop_duplicates().shape[0]
        nx_ratio = self.nx_ratio(df)

        fnreq_max = int(df.fnreq.max())

        with self.config.psyconn.cursor() as cursor:
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

            self.run_fetch()
            if self.dbpcap is None: raise Exception("DB pcap is None")

            print("[info]: pcap successfully inserted with id %s" % self.dbpcap.id) # ignore

            cursor.execute("""CREATE TABLE IF NOT EXISTS public.message_%s PARTITION OF public.message FOR VALUES IN (%s);""", (self.dbpcap.id, self.dbpcap.id, ))
            cursor.connection.commit()

            pass
        pass
    pass

    def run_postgres_dn(self, df: pd.DataFrame):
        df_u = df.drop_duplicates(subset="dn").copy()

        cursor = self.config.psyconn.cursor()

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
            tuples = tuple( row["dn"] for _, row in df_u.iterrows() )
            cursor.execute("""SELECT id from dn WHERE dn.dn IN %s""", (tuples, ))
            pass

        df_u["dn_id"] = [t[0] for t in cursor.fetchall()]
        df = df.merge(df_u[["dn", "dn_id"]], left_on="dn", right_on="dn")

        cursor.connection.commit()
        cursor.close()

        return df

    pass


    def run_postgres_message(self, df: pd.DataFrame):
        if self.dbpcap is None:
            raise Exception("DB pcap cannot be none")

        cursor = self.config.psyconn.cursor()

        values = [
            [
                x["fn"], self.dbpcap.id, x["time"], x["fn"],
                x["fnreq"], x["dn_id"], x["qcode"],
                x["qr"] == "r", x["rcode"], x["server"],
                x["answer"]
            ]
            for _, x in df.iterrows()
        ]
        args_str = b",".join([cursor.mogrify("(%s, %s,%s,%s, %s,%s,%s, %s,%s,%s, %s)", x) for x in values])
        cursor.execute(f"""
            INSERT INTO public.message_{self.dbpcap.id}(
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
        self.run_fetch()

        if self.dbpcap and self.dbpcap.batches and self.dbpcap.batches.complete():
            print("PCAP already processed in database with id %s" % self.dbpcap.id)
            return

        self.run_psl_list()
        self.run_tshark()
        self.run_dns_parse()

        df = pd.read_csv(self.dns_parse_path, index_col=False)

        df.tld.replace([np.nan], [None], inplace=True)
        df.icann.replace([np.nan], [None], inplace=True)
        df.private.replace([np.nan], [None], inplace=True)
        df.server.replace([np.nan], [None], inplace=True)
        df.answer.replace([np.nan], [None], inplace=True)

        df.index.set_names("fn", inplace=True)
        df = df.reset_index()
        
        if not self.dbpcap:
            self.run_insert(df)
            if self.dbpcap is None:
                raise Exception("DB pcap is None")

        if not self.dbpcap.batches:
            self.dbpcap.batches = DBPCAPBatches(0, 2000, math.ceil(df.shape[0] / 2000))
            pass
    
        for batch in range(self.dbpcap.batches.done, self.dbpcap.batches.total):
            batch_start = self.dbpcap.batches.size * batch
            batch_end = self.dbpcap.batches.size * (batch + 1) if (batch + 1) < self.dbpcap.batches.total else df.shape[0]

            print("[info]: processing batch %s/%s..." % (batch + 1, self.dbpcap.batches.total))

            df_batch = df.iloc[batch_start:batch_end]

            df_batch = self.run_postgres_dn(df_batch)

            self.run_postgres_message(df_batch)

            self.dbpcap.batches.done += 1

            with self.config.psyconn.cursor() as cursor:
                cursor.execute("""UPDATE pcap SET batches = %s WHERE id = %s""", (Json(asdict(self.dbpcap.batches)), self.dbpcap.id, ))
                self.config.psyconn.commit()
                pass
            pass

        pass