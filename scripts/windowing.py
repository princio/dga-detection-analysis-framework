
import logging
import math
from pathlib import Path
import sys
from dependency_injector.wiring import Provide, inject
import pandas as pd

sys.path.append(str(Path(__file__).resolve().parent.parent.joinpath('suite2').absolute()))
from suite2.db import Database
from suite2.container import Suite2Container

def query_unique_nx(day: int, window_length: int, idxwindow: int, dac_family: str, eps: str):
    return f"""
    WITH
        W AS (
            SELECT * 
            FROM get_window_all2({day}, {window_length}, {idxwindow}) 
            WHERE
			(DAC_RANK=1 OR DAC_RANK=3)
			AND (DAC_FAMILY={dac_family} OR DAC_FAMILY IS NULL)
            AND RN_QR_RCODE=1
            AND RCODE=3
        ),
        W_DN AS (
            SELECT
				MAC,
                COUNT(*) AS QR,
                SUM((RCODE IS NULL)::INT) AS Q,
                SUM((RCODE = 3)::INT) AS NX
            FROM W
            GROUP BY DN_ID, MAC
        ),
        W_DN_DESCRIBE AS (
            SELECT
				MAC,
                COUNT(*) AS "num DN",
                AVG(QR) AS "avg DN/QR",
                AVG(Q) AS "avg DN/Q",
                AVG(NX) AS "avg DN/NX",
                STDDEV(QR) AS "std DN/QR",
                STDDEV(Q) AS "std DN/Q",
                STDDEV(NX) AS "std DN/NX",
                MAX(QR) AS "max DN/QR",
                MAX(Q) AS "max DN/Q",
                MAX(NX) AS "max DN/NX"
            FROM W_DN
			GROUP BY MAC
        ),
        METRICS AS (
            SELECT
				MAC,
				CEIL(MAX(SECONDS) - MIN(SECONDS))::int AS "seconds",
                COUNT(*) AS QR,
                COUNT(*) FILTER (WHERE DAC_RANK = 1) AS "num DAC_RANK=1",
                COUNT(*) FILTER (WHERE DAC_RANK = 3) AS "num DAC_RANK=3",
                COUNT(*) FILTER (WHERE RCODE IS NULL) AS Q,
                COUNT(*) FILTER (WHERE RCODE = 3 AND {eps} >= 0.5) AS NX,
                COUNT(*) FILTER (WHERE {eps} >= 0.5) AS QR_P1,
                COUNT(*) FILTER (WHERE RCODE IS NULL AND {eps} >= 0.5) AS Q_P1,
                COUNT(*) FILTER (WHERE RCODE = 3 AND {eps} >= 0.5) AS NX_P1
            FROM W
			GROUP BY MAC
        )
        SELECT M.*, DN.*
        FROM METRICS M
        JOIN W_DN_DESCRIBE DN ON M.MAC=DN.MAC
"""


def get_nwindows(db: Database, table_name: str, window_length:int) -> int:
    print(f'Getting windows {table_name}..', end='')
    with db.psycopg2().cursor() as cursor:
        cursor.execute(f'SELECT MAX(seconds) FROM {table_name}')
        seconds_max = cursor.fetchone()[0] # type: ignore
        return math.ceil(seconds_max / window_length)
    print('gotten.')
    pass

def create_index(db: Database, table_name: str):
    print(f'Creating index for {table_name}..', end='')
    with db.psycopg2().cursor() as cursor:
        cursor.execute(f"""
                        CREATE INDEX IF NOT EXISTS {table_name}_floor_idx
                        ON public.{table_name} USING btree
                        (floor(seconds / 360::double precision) ASC NULLS LAST)
                        WITH (deduplicate_items=True)
                        TABLESPACE pg_default;
        """)
        cursor.connection.commit()
        pass
    print('created.')
    pass

def drop_index(db: Database, table_name: str):
    print(f'Dropping index {table_name}..', end='')
    with db.psycopg2().cursor() as cursor:
        cursor.execute(f'DROP INDEX IF EXISTS public.{table_name}_floor_idx')
        cursor.connection.commit()
        pass
    print('dropped.')
    pass

@inject
def main(
        db: Database = Provide[
            Suite2Container.db
        ],
) -> None:
    
    print('Starting.')
    
    nn = "virut"
    nn = "EPS1"
    window_length = 3600
    ndays=10

    file = Path(f'windowing/windows_dac_{nn}_{window_length}_{ndays}.csv')

    dac_families = ['virut', 'modpack', 'healthy', 'necurs', 'pitou', 'conficker', 'suppobox', 'tofsee']

    for idxday in range(10):
        table_name = f'mv3_{idxday}'
        create_index(db, table_name)
        pass

    day0 = 0
    dac_family0 = 0
    idxw0 = 0
    rows = []
    if file.exists():
        df = pd.read_csv(file, index_col=0)
        rows = df.to_numpy().tolist()
        day0 = rows[-1][df.columns.to_list().index('day')]
        dac_family0 = dac_families.index(rows[-1][df.columns.to_list().index('dac_family')])
        idxw0 = rows[-1][df.columns.to_list().index('idxdaywindow')] + 1
        rows= [df]
        pass


    nwindows = math.ceil(3600 * 24 / window_length)
    print(f'EPS={nn}\twindow length={window_length}\tnwindows={nwindows}\tndays={ndays}')
    print(f'start:\tday0={day0}\tdac_family0={dac_family0}\tidxw0={idxw0}')
    print(f'dac families:{",".join(dac_families)}')
    for idx_dac_family in range(dac_family0, len(dac_families)):
        dac_family = dac_families[idx_dac_family]
        for idxday in range(day0, ndays):
            table_name = f'mv3_{idxday}'
            for idxw in range(idxw0, nwindows):
                print('Eseguo get_metrics per tabella %s, dac_family %s, finestra %d/%d' % (table_name, dac_family, idxw, nwindows))
                row = pd.read_sql(f'SELECT * FROM get_metrics_mw(\'{table_name}\', \'{dac_family}\', {window_length}, {idxw}, \'{nn}\')', db.sqlalchemy())
                row.insert(0, 'dac_family', dac_family)
                row.insert(0, 'day', idxday)
                row.insert(0, 'idxdaywindow', idxw)
                row.insert(0, 'idxwindow', idxw + idxday * nwindows)
                rows.append(row)
                pd.concat(rows).reset_index(drop=True).to_csv(file)   
                pass
            idxw0 = 0
            pass
        day0 = 0
        pass
    pass

if __name__ == "__main__":
    application = Suite2Container()
    
    application.config.from_dict({
        "env": "debug",
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
        "logging": logging.INFO
    })

    application.init_resources()

    application.wire(modules=[__name__])

    main()

    application.shutdown_resources()

    pass

"""
DO $$
DECLARE
    counter INTEGER;            -- Per iterare sulle tabelle da 0 a 9
    max_value INTEGER;          -- Per memorizzare il massimo valore di CEIL(seconds/360)
    current_value INTEGER;      -- Per iterare da 0 a max_value
    table_name TEXT;            -- Nome della tabella corrente
    eps1 FLOAT := 0.5;          -- Parametro EPS1
    interval_size INTEGER := 360; -- Parametro per il divisore di seconds
BEGIN
    -- Loop su tutte le tabelle message_{counter} da 0 a 9
    FOR counter IN 0..0 LOOP
        table_name := format('mv3_%s', counter);
        
        -- Ottenere il massimo valore di CEIL(seconds/360) dalla tabella corrente
        EXECUTE format('SELECT COALESCE(MAX(CEIL(seconds/%s)), 0) FROM %I', interval_size, table_name)
        INTO max_value;

        -- Loop da 0 al massimo valore ottenuto
        FOR current_value IN 0..2 LOOP--max_value LOOP
            -- Esegui la funzione get_metrics con i parametri
            RAISE NOTICE 'Eseguo get_metrics per tabella %, intervallo %', table_name, current_value;
            
            -- Chiamata alla funzione get_metrics
            EXECUTE format('SELECT * FROM get_metrics(%s, %s, %s, %L)', 
                           eps1, interval_size, current_value, table_name);
        END LOOP;
    END LOOP;
END $$;
"""