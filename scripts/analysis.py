import logging
from pathlib import Path
import sys
from dependency_injector.wiring import Provide, inject
import pandas as pd

sys.path.append(str(Path(__file__).resolve().parent.parent.joinpath('suite2').absolute()))
from suite2.db import Database
from suite2.dn.dn_service import DNService
from suite2.container import Suite2Container


def query_fps(P_gt, N_gt, packetfilter) -> str:
    query = f"""
WITH GT AS (          
	SELECT
        *,
	    {P_gt} AS P_GT,
        {N_gt} AS N_GT
	FROM
		MESSAGE2_IT2016_0_COMPACT M2
),
TAB AS (
	SELECT
        M2.DN_ID,
		SUM((P_GT)::int) AS P,
		SUM((DNNN.EPS2 < 0.5 AND P_GT)::int) AS FN,
		SUM((DNNN.EPS2 >= 0.5 AND P_GT)::int) AS TP,
	
		SUM((not P_GT)::int) AS N0,
		SUM((DNNN.EPS2 >= 0.5 AND not P_GT)::int) AS FP0,
		SUM((DNNN.EPS2 < 0.5 AND not P_GT)::int) AS TN0,
	
		SUM((N_GT)::int) AS N,
		SUM((DNNN.EPS2 >= 0.5 AND N_GT)::int) AS FP,
		SUM((DNNN.EPS2 < 0.5 AND N_GT)::int) AS TN
	FROM
		GT M2
		JOIN DN_NN_ALL DNNN ON M2.DN_ID = DNNN.DN_ID
		{packetfilter}
    GROUP BY M2.DN_ID
	),
FPS AS (
    SELECT
        DN.DN,
        DN.BDN,
        TAB.*
        FROM TAB JOIN DN ON TAB.DN_ID=DN.id
        -- JOIN DAC ON DN.BDN=DAC.DN
    WHERE FP > 0
    ORDER BY FP DESC
)
SELECT *
FROM FPS
    LEFT JOIN jgamblin J ON FPS.DN=J.DN
WHERE J.ID IS NULL
"""
    return query


def FPR(P_gt, N_gt, packetfilter) -> str:
    query = f"""
WITH GT AS (          
	SELECT
        *,
	    {P_gt} AS P_GT,
        {N_gt} AS N_GT
	FROM
		MESSAGE2_IT2016_0_COMPACT M2
),
TAB AS (
	SELECT
	
		SUM((P_GT)::int) AS P,
		SUM((DNNN.EPS2 < 0.5 AND P_GT)::int) AS FN,
		SUM((DNNN.EPS2 >= 0.5 AND P_GT)::int) AS TP,
	
		SUM((not P_GT)::int) AS N0,
		SUM((DNNN.EPS2 >= 0.5 AND not P_GT)::int) AS FP0,
		SUM((DNNN.EPS2 < 0.5 AND not P_GT)::int) AS TN0,
	
		SUM((N_GT)::int) AS N,
		SUM((DNNN.EPS2 >= 0.5 AND N_GT)::int) AS FP,
		SUM((DNNN.EPS2 < 0.5 AND N_GT)::int) AS TN
	FROM
		GT M2
		JOIN DN_NN_ALL DNNN ON M2.DN_ID = DNNN.DN_ID
		{packetfilter}
	)
SELECT 
	-- n, fp, tn,
	tp::double precision/p AS tpr,
	fp0::double precision/n0 as fpr0,
	fp::double precision/n as fpr
	FROM TAB
"""
    return query


@inject
def main(
        db: Database = Provide[
            Suite2Container.db
        ],
) -> None:
    
    P_gt = "(DN_RANK=1 AND DAC_RANK=1)"
    N_gt = "(DN_RANK=1 AND DAC_RANK=4 OR (DAC_RANK=3 AND WL_RANK<5))"
    packetfilter = "WHERE M2.RN_QR_RCODE=1 AND RCODE=3"
    
    query = query_fps(P_gt, N_gt, packetfilter)

    print('\n', query, '\n')
    
    df = pd.read_sql(query, db.sqlalchemy())
    
    print(df)

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