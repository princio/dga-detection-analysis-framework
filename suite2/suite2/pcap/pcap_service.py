from dataclasses import dataclass
import logging
from pathlib import Path

from .pcap import PCAP

from ..db import Database
from ..pcap.dns_parse_service import DNSParseService
from ..pcap.tcpdump_service import TCPDumpService
from ..pcap.tshark_service import TSharkService
from ..message.message_service import MessageService
from logging import Logger

class PCAPService:
    def __init__(self,
                 db: Database,
                 tcpdump_service: TCPDumpService,
                 tshark_service: TSharkService,
                 dns_parse_service: DNSParseService,
                 message_service: MessageService,
                ) -> None:
        self.db = db
        self.tcpdump_service = tcpdump_service
        self.tshark_service = tshark_service
        self.dns_parse_service = dns_parse_service
        self.message_service = message_service
        pass

    def findby_hash(self, sha256: str):
        with self.db.psycopg2().cursor() as cursor:
            cursor.execute("SELECT id FROM pcap WHERE sha256=%s", ( sha256, ))
            return cursor.fetchone()[0]

    def insert(self, pcapfile: Path, sha256: str, year: int, dataset: str, infected = None):
        with self.db.psycopg2().cursor() as cursor:
            cursor.execute(
                """
                INSERT INTO pcap (
                    name, sha256, infected, dataset, year
                ) VALUES (
                    %s, %s, %s, %s, %s
                )
                RETURNING id;
                """,
                ( pcapfile.name, sha256, infected, dataset, year )
            )
            
            retid = cursor.fetchone()
            if retid is None:
                raise Exception('Returning id is None.')
            retid = retid[0]
            
            logging.info("pcap successfully inserted with id %s" % retid)

            cursor.execute(
                """
                CREATE TABLE IF NOT EXISTS public.message_%s
                PARTITION OF public.message FOR VALUES IN (%s);
                """,
                (retid, retid, )
            )

            # cursor.connection.commit()
            
            return retid
        pass
    pass

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
        o1 = self.tcpdump_service.run(pcapfile)
        o2 = self.tshark_service.run(o1)
        return self.dns_parse_service.run(o2)
    
    def get(self, pcap_id: int) -> PCAP:
        with self.db.psycopg2().cursor() as cursor:
            cursor.execute(
                """
                SELECT id, name, sha256, infected, dataset, year FROM PCAP where id=%s
                """,
                ( pcap_id, )
            )
            pcap = cursor.fetchone()
            if pcap is None:
                raise Exception(f'No pcap found with id {pcap_id}.')
            return PCAP(pcap[0], pcap[1], pcap[2], pcap[3], pcap[4], pcap[5])
        pass

    def new(self, pcapfile: Path, sha256: str, year: int, dataset: str, infected = None):
        csvfile = self.process(pcapfile)

        pcap_id = self.insert(pcapfile, sha256, year, dataset, infected)

        self.message_service.insert_pcapcsvfile(pcap_id, csvfile)
    pass