WITH DNBIG AS (
    SELECT
        DN.ID,
        DN.REGEX_CHECK,
        DN_NN.VALUE AS EPS,
        W1.ID AS WLID
    FROM
        DN
        LEFT JOIN (
            SELECT
                DN_ID,
                VALUE
            FROM
                DN_NN
            WHERE
                DN_NN.NN_ID = 1
        ) DN_NN ON DN_NN.DN_ID = DN.ID
        LEFT JOIN WHITELIST1 W1 ON DN_NN.DN_ID = W1.DN_ID
),
M2 AS (
    SELECT
        COUNT(*) AS COUNTER,
        DN_ID,
        PCAP_ID,
        MIN(
            PCAP.TIME_S_MIN + '00:00:01' :: INTERVAL * EXTRACT(
                EPOCH
                FROM
                    M2.TIME_S
            ) :: DOUBLE PRECISION
        ) AS TIME_S
    FROM
        MESSAGE2 M2
        JOIN PCAP ON M2.PCAP_ID = PCAP.ID -- JOIN PCAP ON M2.PCAP_ID = PCAP.ID
        -- LEFT JOIN DAC_DN ON M2.DN_ID = DAC_DN.DN_ID
        -- LEFT JOIN DN_NN ON M2.DN_ID = DN_NN.DN_ID
        -- WHERE IS_R is TRUE
    GROUP BY
        DN_ID,
        PCAP_ID
),
M22 AS (
    SELECT
        *,
        (DAC_DN.DN_ID is not null) as lbl,
        -- 1 is positive
        (DNBIG.EPS >= 0.5) as cls,
        (DNBIG.WLID is not null) as wl,
        (DNBIG.regex_check) as isvalid,
        true
    FROM
        M2
        JOIN DNBIG ON M2.DN_ID = DNBIG.ID
        LEFT JOIN DAC_DN ON M2.DN_ID = DAC_DN.DN_ID
        AND (
            time_s BETWEEN DAC_DN.TS_BEGIN
            AND DAC_DN.TS_END
        )
),
M222 as (
    SELECT
        PCAP.NAME,
        PCAP.TIME_S_MIN,
        COUNT(*) as tot,
        count(*) filter (
            where
                not lbl
        ) as n,
        count(*) filter (
            where
                lbl
        ) as p,
        count(*) filter (
            where
                not lbl
                and not cls
        ) as tn,
        count(*) filter (
            where
                lbl
                and cls
        ) as tp,
        count(*) filter (
            where
                not lbl
                and cls
        ) as fp,
        count(*) filter (
            where
                not lbl
                and cls
                and wl
        ) as fp_wl
    FROM
        M22
        JOIN PCAP ON M22.PCAP_ID = PCAP.ID -- WHERE isvalid
    GROUP BY
        PCAP.NAME,
        PCAP.TIME_S_MIN
    ORDER BY
        PCAP.TIME_S_MIN
)
SELECT
    *,
    CASE
        WHEN N > 0 THEN TN :: double precision / N
        ELSE null
    END as tnr,
    CASE
        WHEN P > 0 THEN TP :: double precision / P
        ELSE null
    END as tpr
FROM
    M222 -- 132626748
    -- 16026856
    -- 1379624
    -- 91424
    -- TUTTI CON DISTINCT ON:
    -- 7967984
    -- 282428
    -- SOLO LE REQUESTs CON DISTINCT ON:
    -- 240948
    -- 7365668
    -- SOLO LE RESPONSE CON DISTINCT ON:
    -- 282892
    -- 7966020