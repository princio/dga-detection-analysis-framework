import logging
from pathlib import Path
from dependency_injector.wiring import Provide, inject

from suite2.db import Database
from suite2.dn.dn_service import DNService
from suite2.container import Suite2Container


import multiprocessing
import time

def query(partition: str):
    return f"""
DROP MATERIALIZED VIEW IF EXISTS public.{partition}_compact;

CREATE MATERIALIZED VIEW IF NOT EXISTS public.{partition}_compact
TABLESPACE pg_default
AS
 WITH mm0 AS (
         SELECT m2.id,
            EXTRACT(epoch FROM m2.time_s) AS seconds,
            m2.pcap_id,
            m2.dn_id,
                CASE
                    WHEN m2.is_r THEN m2.rcode
                    ELSE NULL::integer
                END AS rcode,
                CASE
                    WHEN m2.is_r THEN m2.dst
                    ELSE m2.src
                END AS terminal,
            row_number() OVER (PARTITION BY m2.dn_id ORDER BY m2.time_s) AS rn,
            row_number() OVER (PARTITION BY m2.dn_id, m2.is_r ORDER BY m2.time_s) AS rn_qr,
            row_number() OVER (PARTITION BY m2.dn_id, m2.is_r, m2.rcode ORDER BY m2.time_s) AS rn_qr_rcode,
            row_number() OVER (
                PARTITION BY
                    m2.dn_id, (CASE WHEN m2.is_r THEN m2.dst ELSE m2.src END)
                ORDER BY
                    m2.time_s)
                AS rn_terminal,
            row_number() OVER (
                PARTITION BY
                    m2.dn_id, (CASE WHEN m2.is_r THEN m2.dst ELSE m2.src END), m2.is_r
                ORDER BY
                    m2.time_s)
                AS rn_terminal_qr,
            row_number() OVER (
                PARTITION BY
                    m2.dn_id, (CASE WHEN m2.is_r THEN m2.dst ELSE m2.src END), m2.is_r, m2.rcode
                ORDER BY m2.time_s)
                AS rn_terminal_qr_rcode
           FROM {partition} m2
          ORDER BY m2.time_s
        ), mm1 AS (
         SELECT m2.id,
            m2.seconds,
            m2.pcap_id,
            m2.dn_id,
            m2.rcode,
            m2.terminal,
            m2.rn,
            m2.rn_qr,
            m2.rn_qr_rcode,
            m2.rn_terminal,
            m2.rn_terminal_qr,
            m2.rn_terminal_qr_rcode,
            dac_dn.dac_id,
            dac_dn.ts_begin,
            dac_dn.ts_end,
                CASE
                    WHEN dac_dn.dac_id IS NOT NULL THEN
                    CASE
                        WHEN (pcap.time_s_min + '00:00:01'::interval * m2.seconds::double precision) >= dac_dn.ts_begin AND (pcap.time_s_min + '00:00:01'::interval * m2.seconds::double precision) <= dac_dn.ts_end THEN 1
                        WHEN dac_dn.ts_begin >= '2016-04-20 00:00:00'::timestamp without time zone AND dac_dn.ts_begin <= '2016-05-15 00:00:00'::timestamp without time zone OR dac_dn.ts_end >= '2016-04-20 00:00:00'::timestamp without time zone AND dac_dn.ts_end <= '2016-05-15 00:00:00'::timestamp without time zone THEN 2
                        ELSE 3
                    END
                    ELSE 4
                END AS dac_rank,
                CASE
                    WHEN w1.id IS NOT NULL THEN
                    CASE
                        WHEN w1.rank_bdn < 1000 THEN 1
                        WHEN w1.rank_bdn < 10000 THEN 2
                        WHEN w1.rank_bdn < 100000 THEN 3
                        ELSE 4
                    END
                    ELSE 5
                END AS wl_rank,
                CASE
                    WHEN dn.regex_check THEN 1
                    WHEN dn.psltrie_rcode >= 0 THEN 2
                    ELSE 3
                END AS dn_rank
           FROM mm0 m2
             JOIN pcap ON m2.pcap_id = pcap.id
             JOIN dn ON m2.dn_id = dn.id
             LEFT JOIN whitelist1 w1 ON w1.dn_id = dn.id
             LEFT JOIN dac_dn dac_dn ON dn.id = dac_dn.dn_id
        ), mm2 AS (
         SELECT mm1.id,
            mm1.seconds,
            mm1.pcap_id,
            mm1.dn_id,
            mm1.rcode,
            mm1.terminal,
            mm1.rn,
            mm1.rn_qr,
            mm1.rn_qr_rcode,
            mm1.rn_terminal,
            mm1.rn_terminal_qr,
            mm1.rn_terminal_qr_rcode,
            mm1.dac_id,
            mm1.ts_begin,
            mm1.ts_end,
            mm1.dac_rank,
            mm1.wl_rank,
            mm1.dn_rank,
            row_number() OVER (PARTITION BY mm1.id ORDER BY mm1.dac_rank, mm1.dac_id) AS rn_dacrank
           FROM mm1
        )
 SELECT id,
    seconds,
    pcap_id,
    dn_id,
    rcode,
    terminal,
    rn,
    rn_qr,
    rn_qr_rcode,
    rn_terminal,
    rn_terminal_qr,
    rn_terminal_qr_rcode,
    dac_id,
    ts_begin,
    ts_end,
    dac_rank,
    wl_rank,
    dn_rank
   FROM mm2
  WHERE rn_dacrank = 1
  ORDER BY id
WITH DATA;

ALTER TABLE IF EXISTS public.{partition}_compact OWNER TO postgres;
"""

def conta_tempo():
    start_time = time.time()
    while True:
        elapsed_time = time.time() - start_time
        print(f"\rTempo trascorso: {elapsed_time:.1f} secondi", end='', flush=True)
        time.sleep(0.5)
        pass
    pass


@inject
def main(
        db: Database = Provide[
            Suite2Container.db
        ],
) -> None:
    for day in range(1, 10):
        p = multiprocessing.Process(target=conta_tempo)
        partition = f"message2_it2016_{day}"
        print("Doing partition %s" % partition)
        p.start()
        with db.psycopg2().cursor() as cursor:
            cursor.execute(query(partition))
        print("Done partition %s." % partition)
        p.terminate()
        p.join()
    return


if __name__ == "__main__":

    application = Suite2Container()
    
    application.config.from_dict({
        "env": "prod",
        "db": {
            "host": "localhost",
            "user": "postgres",
            "password": "postgre",
            "dbname": "dns_mac",
            "port": 5432
        },
        "binaries": {
            "tshark": "/opt/homebrew/bin/tshark",
            "tcpdump": "/usr/sbin/tcpdump",
            "dns_parse": "/Users/princio/Repo/princio/malware-detection-predict-file/dns_parse/bin/dns_parse",
            "psltrie": "/Users/princio/Repo/princio/malware-detection-predict-file/psltrie/bin/binary_prod",
            "iconv": "/usr/bin/iconv"
        },
        "workdir": "/tmp/suite2_workdir",
        "logging": logging.DEBUG
    })

    application.init_resources()

    application.wire(modules=[__name__])

    main()

    application.shutdown_resources()

    pass