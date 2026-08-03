
from ipaddress import collapse_addresses
import logging
import math
from pathlib import Path
from dependency_injector.wiring import Provide, inject
import pandas as pd

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
			AND (DAC_FAMILY='{dac_family}' OR DAC_FAMILY IS NULL)
            AND RN_QR_RCODE=1
            AND RCODE IS NULL
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
				MAC AS MAC2,
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
        JOIN W_DN_DESCRIBE DN ON M.MAC=DN.MAC2
"""

@inject
def main(
        db: Database = Provide[
            Suite2Container.db
        ],
) -> None:
    
    print('Starting.')

    U='queries'
    nn = "EPS1"
    window_length = 3600
    ndays=10

    dirs = Path(f'../output/windowing_py/')
    dirs.mkdir(parents=True, exist_ok=True)
    file = dirs.joinpath(f'windows_ALL2_dac_{nn}_{window_length}_{ndays}_unique{U}.csv')

    dac_families = sorted(['virut', 'modpack', 'necurs', 'pitou', 'conficker', 'suppobox', 'tofsee'])
    dac_families.append('healthy')

    day0 = 0
    dac_family0 = 0
    idxw0 = 0
    rows = []
    DF = pd.DataFrame()
    if file.exists():
        df = pd.read_csv(file, index_col=0)
        rows = df.to_numpy().tolist()
        day0 = rows[-1][df.columns.to_list().index('day')]
        dac_family0 = dac_families.index(rows[-1][df.columns.to_list().index('dac_family')])
        idxw0 = rows[-1][df.columns.to_list().index('idxdaywindow')] + 1
        df.reset_index(drop=True, inplace=True)
        DF = df
        pass

    if 'mac.1' in DF.columns:
        if not (DF['mac'] == DF['mac.1']).all():
            print('Errore: le colonne mac e mac.1 non coincidono!')
            exit(1)
        else:
            DF.drop(columns=['mac.1'], inplace=True)
            pass
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
                df = pd.read_sql(query_unique_nx(idxday, window_length, idxw, dac_family, nn), db.sqlalchemy())
                df.insert(0, 'dac_family', dac_family)
                df.insert(0, 'day', idxday)
                df.insert(0, 'idxdaywindow', idxw)
                df.insert(0, 'idxwindow', idxw + idxday * nwindows)
                df.drop(columns=['mac2'], inplace=True)

                DF = pd.concat([ DF, df ], ignore_index=True)
                DF.to_csv(file)
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
            "user": "princio",
            "password": "postgres",
            "dbname": "ti2016",
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