-- FUNCTION: public.get_metrics_mw(text, text, integer, integer, text)

-- DROP FUNCTION IF EXISTS public.get_metrics_mw(text, text, integer, integer, text);

CREATE OR REPLACE FUNCTION public.get_metrics_mw(
	table_name text,
	malware_family text,
	window_length integer,
	idxwindow integer,
	eps1 text)
    RETURNS TABLE(mac text, seconds integer, qr bigint, "num DAC_RANK=1" bigint, "num DAC_RANK=3" bigint, q bigint, nx bigint, qr_p1 bigint, q_p1 bigint, nx_p1 bigint, mac2 text, "num DN" bigint, "avg DN/QR" numeric, "avg DN/Q" numeric, "avg DN/NX" numeric, "std DN/QR" numeric, "std DN/Q" numeric, "std DN/NX" numeric, "max DN/QR" bigint, "max DN/Q" bigint, "max DN/NX" bigint) 
    LANGUAGE 'plpgsql'
    COST 100
    VOLATILE PARALLEL UNSAFE
    ROWS 1000

AS $BODY$
DECLARE
    query TEXT;
BEGIN
    -- Costruisci la query dinamicamente
    query := format($f$
        WITH
        W AS (
            SELECT * 
            FROM %I 
            WHERE
			FLOOR(SECONDS / %s) = %s
			AND (DAC_RANK=1 OR DAC_RANK=3)
			AND (DAC_FAMILY=%L OR DAC_FAMILY IS NULL)
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
                COUNT(*) FILTER (WHERE RCODE = 3 AND %s >= 0.5) AS NX,
                COUNT(*) FILTER (WHERE %s >= 0.5) AS QR_P1,
                COUNT(*) FILTER (WHERE RCODE IS NULL AND %s >= 0.5) AS Q_P1,
                COUNT(*) FILTER (WHERE RCODE = 3 AND %s >= 0.5) AS NX_P1
            FROM W
			GROUP BY MAC
        )
        SELECT M.*, DN.*
        FROM METRICS M
        JOIN W_DN_DESCRIBE DN ON M.MAC=DN.MAC
    $f$, table_name, window_length, idxwindow, malware_family, eps1, eps1, eps1, eps1);

    -- Esegui la query dinamicamente
    RETURN QUERY EXECUTE query;
END;
$BODY$;

ALTER FUNCTION public.get_metrics_mw(text, text, integer, integer, text)
    OWNER TO postgres;
