-- View: public.message2_it2016_0_compact

-- REFRESH MATERIALIZED VIEW public.dac_dn WITH DATA;

DROP MATERIALIZED VIEW IF EXISTS public.message2_it2016_0_compact;

CREATE MATERIALIZED VIEW IF NOT EXISTS public.message2_it2016_0_compact
TABLESPACE pg_default
AS
  WITH 
	 mm0 AS (
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
            row_number() OVER (PARTITION BY m2.dn_id, m2.is_r, m2.rcode ORDER BY m2.time_s) AS rn_qr_rcode
           FROM message2_it2016_0 m2
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
            dac_dn.dac_id AS dac_id,
            dac_dn.ts_begin,
            dac_dn.ts_end,
                CASE
                    WHEN dac.dac_id IS NOT NULL THEN
                    CASE
                        WHEN (pcap.time_s_min + '00:00:01'::interval * m2.seconds::double precision) BETWEEN dac_dn.ts_begin AND dac_dn.ts_end THEN 1
                        WHEN dac_dn.ts_begin BETWEEN '2016-04-20 00:00:00.000000' AND '2016-05-15 00:00:00.000000' OR dac_dn.ts_end BETWEEN '2016-04-20 00:00:00.000000' AND '2016-05-15 00:00:00.000000' THEN 2
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
                    WHEN dn.psltrie_rcode <> '-1'::integer THEN 2
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

ALTER TABLE IF EXISTS public.message2_it2016_0_compact
    OWNER TO postgres;