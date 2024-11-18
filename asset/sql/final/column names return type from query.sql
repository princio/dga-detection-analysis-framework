drop table if exists tmptmp;

create temp table if not exists tmptmp as (SELECT
	pcap.day,
	rn.rn,
	rn.rn_qr,
	rn.rn_qr_rcode,
	rn.rn_mac,
	rn.rn_mac_qr,
	rn.rn_mac_qr_rcode,
	w.*
FROM
	MESSAGE3_IT2016_0 W
	JOIN M3_RN RN ON W.ID = RN.ID
	AND W.PCAP_ID = RN.PCAP_ID
	JOIN PCAP ON W.PCAP_ID = PCAP.ID
WHERE
	FLOOR(W.SECONDS / 3600) = 0 LIMIT 1);

SELECT column_name, data_type
FROM   information_schema.columns
WHERE  table_name = 'tmptmp'
ORDER  BY ordinal_position;

-- ANALYZE m3_rn;
-- select * from message3_it2016_1 w join m3_rn on w.id=m3_rn.id and w.pcap_id=w.pcap_id 
-- where
-- 	FLOOR(w.SECONDS / 3600) = 0;
-- 28s
-- WITH m2 as MATERIALIZED (select m2.* from message3_it2016_1 m2 where 
-- 	FLOOR(SECONDS / 3600) = 0
-- )
-- , w as MATERIALIZED (
-- 	SELECT
-- 		m2.id,
-- 	    m2.seconds,
-- 	    m2.pcap_id,
-- 	    m2.dn_id,
--         CASE
--             WHEN cardinality(bigdn_m3.dac_families) > 0 THEN bigdn_m3.dac_families[1]
--             ELSE NULL::text
--         END AS dac_family,
--         CASE
--             WHEN m2.is_r THEN m2.rcode
--             ELSE NULL::integer
--         END AS rcode,
--         CASE
--             WHEN m2.is_r THEN m2.macdst
--             ELSE m2.macsrc
--         END AS mac,
--         CASE
--             WHEN m2.is_r THEN m2.dst
--             ELSE m2.src
--         END AS ip,
--         CASE
--             WHEN cardinality(bigdn_m3.dac_ids) > 0 THEN
--             CASE
--                 WHEN bigdn_m3.dac_count_between > 0 THEN 1
--                 ELSE 2
--             END
--             ELSE 3
--         END AS dac_rank,
--         CASE
--             WHEN bigdn_m3.wl_id IS NOT NULL THEN
--             CASE
--                 WHEN bigdn_m3.wl_rank_bdn IS NOT NULL THEN
--                 CASE
--                     WHEN bigdn_m3.wl_rank_bdn < 1000 THEN 1
--                     WHEN bigdn_m3.wl_rank_bdn < 10000 THEN 2
--                     WHEN bigdn_m3.wl_rank_bdn < 100000 THEN 3
--                     ELSE 4
--                 END
--                 ELSE 5
--             END
--             ELSE 6
--         END AS wl_rank,
--         CASE
--             WHEN bigdn_m3.regex_check THEN 1
--             WHEN bigdn_m3.psltrie_rcode >= 0 THEN 2
--             ELSE 3
--         END AS dn_rank,
--     bigdn_m3.eps1,
--     bigdn_m3.eps2,
--     bigdn_m3.eps3,
--     bigdn_m3.eps4,
--     bigdn_m3.logit1 AS lambda1,
--     bigdn_m3.logit2 AS lambda2,
--     bigdn_m3.logit3 AS lambda3,
--     bigdn_m3.logit4 AS lambda4
--    FROM m2
--      JOIN pcap ON m2.pcap_id = pcap.id
--      JOIN bigdn_m3_2 bigdn_m3 ON m2.dn_id = bigdn_m3.id
-- )
-- select * from w join m3_rn_1 on w.pcap_id=w.pcap_id and w.id=m3_rn_1.id