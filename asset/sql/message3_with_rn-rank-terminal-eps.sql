-- View: public.m3_rn_rank_eps
DROP VIEW public.m3_rn_rank_eps;

CREATE
OR REPLACE VIEW public.m3_rn_rank_eps AS
    SELECT
        m2.id,
        m2.seconds,
        m2.pcap_id,
        m2.dn_id,
        CASE
            WHEN m2.is_r THEN m2.rcode
            ELSE NULL :: integer
        END AS rcode,
        CASE
            WHEN m2.is_r THEN m2.macdst
            ELSE m2.macsrc
        END AS mac,
        CASE
            WHEN m2.is_r THEN m2.dst
            ELSE m2.src
        END AS ip,
        CASE
            WHEN cardinality(bigdn_m3.dac_ids) > 0 THEN CASE
                WHEN bigdn_m3.dac_count_between > 0 THEN 1
                ELSE 2
            END
            ELSE 3
        END AS dac_rank,
        CASE
            WHEN bigdn_m3.wl_id IS NOT NULL THEN CASE
                WHEN bigdn_m3.wl_rank_bdn IS NOT NULL THEN CASE
                    WHEN bigdn_m3.wl_rank_bdn < 1000 THEN 1
                    WHEN bigdn_m3.wl_rank_bdn < 10000 THEN 2
                    WHEN bigdn_m3.wl_rank_bdn < 100000 THEN 3
                    ELSE 4
                END
                ELSE 5
            END
            ELSE 6
        END AS wl_rank,
        CASE
            WHEN bigdn_m3.regex_check THEN 1
            WHEN bigdn_m3.psltrie_rcode >= 0 THEN 2
            ELSE 3
        END AS dn_rank,

        bigdn_m3.eps1 >= 0.5 :: double precision AS pp1,
        bigdn_m3.eps2 >= 0.5 :: double precision AS pp2,
        bigdn_m3.eps3 >= 0.5 :: double precision AS pp3,
        bigdn_m3.eps4 >= 0.5 :: double precision AS pp4,

        row_number() OVER (
            PARTITION BY m2.dn_id
            ORDER BY
                m2.ts
        ) AS rn,
        row_number() OVER (
            PARTITION BY m2.dn_id,
            m2.is_r
            ORDER BY
                m2.ts
        ) AS rn_qr,
        row_number() OVER (
            PARTITION BY m2.dn_id,
            m2.is_r,
            m2.rcode
            ORDER BY
                m2.ts
        ) AS rn_qr_rcode,
        row_number() OVER (
            PARTITION BY m2.dn_id,
            (
                CASE
                    WHEN m2.is_r THEN m2.dst
                    ELSE m2.src
                END
            )
            ORDER BY
                m2.ts
        ) AS rn_terminal,
        row_number() OVER (
            PARTITION BY m2.dn_id,
            (
                CASE
                    WHEN m2.is_r THEN m2.dst
                    ELSE m2.src
                END
            ),
            m2.is_r
            ORDER BY
                m2.ts
        ) AS rn_terminal_qr,
        row_number() OVER (
            PARTITION BY m2.dn_id,
            (
                CASE
                    WHEN m2.is_r THEN m2.dst
                    ELSE m2.src
                END
            ),
            m2.is_r,
            m2.rcode
            ORDER BY
                m2.ts
        ) AS rn_terminal_qr_rcode,

        row_number() OVER (
            PARTITION BY mm1.id
            ORDER BY
                CASE
            WHEN cardinality(bigdn_m3.dac_ids) > 0 THEN CASE
                WHEN bigdn_m3.dac_count_between > 0 THEN 1
                ELSE 2
            END
            ELSE 3
        END
        ) AS rn_dacrank
    FROM
        message3_it2016_0 m2
        JOIN pcap ON m2.pcap_id = pcap.id
        JOIN bigdn_m3 ON m2.dn_id = bigdn_m3.id
)
select * from 
ALTER TABLE
    public.m3_rn_rank_eps OWNER TO postgres;