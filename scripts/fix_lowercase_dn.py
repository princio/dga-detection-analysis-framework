import logging
from pathlib import Path
import sys
from dependency_injector.wiring import Provide, inject

sys.path.append(str(Path(__file__).resolve().parent.parent.joinpath('suite2').absolute()))
from suite2.dn.dn_service import DNService
from suite2.pcap.pcap_service import PCAPService
from suite2.container import Suite2Container


def getone(cursor, query):
    cursor.execute(query)
    return cursor.fetchone()[0]

@inject
def test_pcap(
        pcap_service: PCAPService = Provide[
            Suite2Container.pcap_service
        ],
        dn_service: DNService = Provide[
            Suite2Container.dn_service
        ],
) -> None:
    with pcap_service.db.psycopg2().cursor() as cursor:
        cursor.execute('''
        WITH
        DNN AS (
            SELECT
                ID,
                DN
            FROM
                DN
            WHERE
                DN.DN != LOWER(DN.DN) OR DN.DN = LOWER(DN.DN)
        ),
        DNN_COUNT AS (
            SELECT
                DNN.ID AS ID,
                DNN.DN AS DN,
                ROW_NUMBER() OVER (
                    PARTITION BY
                        LOWER(DNN.DN)
                    ORDER BY
                        DNN.DN
                ) AS ROW_NUM
            FROM
                DNN
            ORDER BY
                DNN.DN,
                ROW_NUM
        ),
        DNS_2CHANGE AS (
            SELECT
                LOWER(DNN_COUNT.DN) AS DN,
                ARRAY_AGG(DNN_COUNT.ID) AS IDS,
                ARRAY_AGG(DNN_COUNT.DN) AS DNS,
                ARRAY_AGG(ROW_NUM) NUM,
                COUNT(*) AS CCOUNT
            FROM
                DNN_COUNT
            WHERE
                ROW_NUM > 1
            GROUP BY
                LOWER(DNN_COUNT.DN)
        ),
        DNS_2KEEP AS (
            SELECT
                DNN_COUNT.ID AS ID,
                DNN_COUNT.DN AS DN
            FROM
                DNN_COUNT
            WHERE
                ROW_NUM = 1
        ),
        DNUPDATE AS (
            SELECT
                DNs_2KEEP.ID AS ID_2KEEP,
                DNs_2KEEP.DN AS DN_2KEEP,
                DNs_2CHANGE.IDS AS IDS_2CHANGE,
                DNs_2CHANGE.DNS AS DNS_2CHANGE,
                DNs_2CHANGE.NUM AS NUM_2CHANGE,
                DNs_2CHANGE.CCOUNT
            FROM
                DNS_2KEEP
                JOIN DNS_2CHANGE ON LOWER(DNS_2KEEP.DN) = DNS_2CHANGE.DN
            WHERE DNs_2CHANGE.CCOUNT > 0
            ORDER BY CCOUNT DESC
        ) SELECT * FROM DNUPDATE ORDER BY CCOUNT''')
        rows = cursor.fetchall()
        c = 0
        for row in rows:
            print(f'\n{c}/{len(rows)}')
            c+=1
            i = 0
            id_2keep = row[i]
            i += 1
            dn_2keep = row[i]
            i += 1
            ids_2change = row[i]
            i += 1
            dns_2change = row[i]
            i += 1
            num_2change = row[i]
            i += 1
            count = row[i]
            i += 1
            print(f'for `{dn_2keep.lower()}` keep id {id_2keep}')
            print(f'  further versions number are {len(ids_2change)}')

            if len(ids_2change) == 1:
                m2keep = getone(cursor, cursor.mogrify("select count(*) from message where dn_id=%s", (id_2keep,)))
                m2change = getone(cursor, cursor.mogrify("select count(*) from message where dn_id=%s", (ids_2change[0],)))
                if m2keep < m2change:
                    tmp = [id_2keep]
                    id_2keep = ids_2change[0]
                    ids_2change = tmp
                print(f'messages for keep {m2keep} while messages for change {m2change}: {"swapped" if m2keep < m2change else "not swapped"}')
                if m2change > 0:
                    cursor.execute(cursor.mogrify("UPDATE MESSAGE M SET DN_ID=%s WHERE M.DN_ID=%s", (id_2keep, ids_2change[0], )))
            else:
                cursor.execute(cursor.mogrify("UPDATE MESSAGE M SET DN_ID=%s FROM DN WHERE M.DN_ID=DN.ID AND DN.DN ILIKE %s", (id_2keep, dn_2keep.lower(), )))
                cursor.connection.commit()
                pass

            cursor.execute(cursor.mogrify("DELETE FROM DN_NN WHERE DN_ID=ANY(%s)"), (ids_2change,))
            cursor.execute(cursor.mogrify("DELETE FROM WHITELIST_DN WHERE DN_ID=ANY(%s)"), (ids_2change,))
            cursor.execute(cursor.mogrify("DELETE FROM DN WHERE ID=ANY(%s)"), (ids_2change,))
            cursor.execute(cursor.mogrify("UPDATE DN SET DN='tmp' WHERE ID=%s", (id_2keep, )))
            cursor.execute(cursor.mogrify("UPDATE DN SET DN=LOWER(%s) WHERE ID=%s", (dn_2keep, id_2keep, )))

            cursor.connection.commit()
        pass

    dn_service.dbfill()
    pass


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

    test_pcap()

    application.shutdown_resources()

    pass