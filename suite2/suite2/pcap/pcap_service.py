from dataclasses import dataclass
import logging
from pathlib import Path
import subprocess
from tempfile import TemporaryFile
from typing import List

import pandas as pd
import psycopg2

from suite2.dn.dn_service import DNService

from ..subprocess.subprocess_service import SubprocessService

from .pcap import PCAP

from ..db import Database
from ..message.message_service import MessageService

class PCAPService:
    def __init__(self,
                 db: Database,
                 subprocess_service: SubprocessService,
                 message_service: MessageService,
                 dn_service: DNService,
                ) -> None:
        self.db = db
        self.subprocess_service = subprocess_service
        self.message_service = message_service
        self.dn_service = dn_service
        pass

    def findby_hash(self, sha256: str):
        with self.db.psycopg2().cursor() as cursor:
            cursor.execute("SELECT id FROM pcap WHERE sha256=%s", ( sha256, ))
            return cursor.fetchone()[0] # type: ignore

    def insert(self, pcapfile: Path, sha256: str, year: int, dataset: str, infected = None):
        with self.db.psycopg2().cursor() as cursor:
            cursor.execute(
                """
                INSERT INTO pcap ( name, sha256, infected, dataset, year )
                VALUES ( %s, %s, %s, %s, %s )
                RETURNING id;
                """,
                ( pcapfile.name, sha256, infected, dataset, year )
            )
            pcap_id = cursor.fetchone()[0] # type: ignore
            logging.getLogger(__name__).info("pcap successfully inserted with id %s" % pcap_id)
            return pcap_id
        pass

    def set_time_min(self, pcap_id: int, time_s_min):
        with self.db.psycopg2().cursor() as cursor:
            cursor.execute("UPDATE pcap SET time_s_min=%s WHERE id=%s", (time_s_min, pcap_id,))
            if cursor.rowcount == 0:
                raise Exception('Impossible to update time_s_min of pcap %d.' % pcap_id)
            pass
        pass

    def processed_path(self, pcapname: str) -> Path:
        return self.subprocess_service.workdir.joinpath('dns_parse').joinpath(f'{pcapname}.tcpdump.tshark.dns_parse')

    def process(self, pcapfile: Path) -> Path:
        """Process a pcap file with the following consequential steps:
        - tcpdump: filtering DNS packets using `port 53` filter (fast).
        - tshark: further filter step (slow).
        - dns_parse: generate the csv.

        Args:
            pcapfile (Path): The path to the pcap file.

        Returns:
            Path: The output path of the csv files.
        """
        o1 = self.subprocess_service.launch_tcpdump(pcapfile)
        o2 = self.subprocess_service.launch_tshark(o1)
        return self.subprocess_service.launch_dns_parse(o2)

    
    def get(self, pcap_id: int) -> PCAP:
        with self.db.psycopg2().cursor() as cursor:
            cursor.execute(
                "SELECT id, name, sha256, infected, dataset, year FROM PCAP where id=%s",
                ( pcap_id, )
            )
            pcap = cursor.fetchone()
            if pcap is None:
                raise Exception(f'No pcap found with id {pcap_id}.')
            return PCAP(pcap[0], pcap[1], pcap[2], pcap[3], pcap[4], pcap[5])
        pass

    def getbyname(self, name: str) -> PCAP:
        with self.db.psycopg2().cursor() as cursor:
            cursor.execute(
                "SELECT id, name, sha256, infected, dataset, year FROM PCAP where name=%s",
                ( name, )
            )
            pcap = cursor.fetchone()
            if pcap is None:
                raise Exception(f'No pcap found with name «{name}».')
            return PCAP(pcap[0], pcap[1], pcap[2], pcap[3], pcap[4], pcap[5])
        pass

    def add(self, pcapfile):
        sha256 = subprocess.getoutput(f"shasum -a 256 {pcapfile}").split(' ')[0]
        try:
            pcap_id = self.insert(pcapfile, sha256, 2016, 'IT16', infected=None)
        except psycopg2.errors.UniqueViolation as e:
            logging.getLogger(__name__).warning(e)
            e.cursor.connection.rollback() # type: ignore
            pcap_id = self.findby_hash(sha256)
            pass
        return pcap_id
    
    def new_partition(self, pcapfiles: List[Path], partition_name: str):
        pcap_ids = []
        dfs = []
        for pcapfile in pcapfiles:
            sha256 = subprocess.getoutput(f"shasum -a 256 {pcapfile}").split(' ')[0]
            try:
                pcap_id = self.insert(pcapfile, sha256, 2016, 'IT16', infected=None)
            except psycopg2.errors.UniqueViolation as e:
                e.cursor.connection.rollback() # type: ignore
                pcap_id = self.findby_hash(sha256)
                pass
            pcap_ids.append(pcap_id)
            pass

        if len(pcap_ids) == 0:
            raise Exception("Number of pcap is 0.")

        for idx, pcapfile in enumerate(pcapfiles):
            pcap_id = pcap_ids[idx]
            outputcsvfile = self.process(pcapfile)
            df = pd.read_csv(outputcsvfile)
            # df = df.head(1000)
            df['dn_id'] = self.dn_service.add(df["dn"]) # important
            dfs.append(self.message_service.dns_parse_preprocess(df, pcap_id))
            pass

        self.message_service.create_partition(partition_name, pcap_ids)
        df = pd.concat(dfs)
        self.message_service.copy(partition_name, df)
        
        for pcapfile in pcapfiles:
            self.subprocess_service.clean(pcapfile)
            pass
        pass
    pass