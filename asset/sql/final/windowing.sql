select
	FLOOR(seconds / 360) as "window",
	COUNT(*) filter (where dac_rank = 1) AS "label_1",
	COUNT(*) filter (where dac_rank <= 2) AS "label_2",
	COUNT(*) as qr,
	count(*) filter (where rcode is null) as q,
	count(*) filter (where rcode = 3) as nx,
	max()
	count(*) filter (where eps1 >= 0.5) as p1,
	count(*) filter (where eps2 >= 0.5) as p2,
	count(*) filter (where eps3 >= 0.5) as p3,
	count(*) filter (where eps4 >= 0.5) as p4,
	sum(CASE WHEN lambda1 = 'infinity' THEN 50 ELSE (CASE WHEN lambda1 = '-infinity' THEN -50 ELSE lambda1 END) END) as llr1,
	sum(CASE WHEN lambda2 = 'infinity' THEN 50 ELSE (CASE WHEN lambda2 = '-infinity' THEN -50 ELSE lambda2 END) END) as llr2,
	sum(CASE WHEN lambda3 = 'infinity' THEN 50 ELSE (CASE WHEN lambda3 = '-infinity' THEN -50 ELSE lambda3 END) END) as llr3,
	sum(CASE WHEN lambda4 = 'infinity' THEN 50 ELSE (CASE WHEN lambda4 = '-infinity' THEN -50 ELSE lambda4 END) END) as llr4
 FROM mv3_0
 WHERE
	-- filter the malware:
	dac_family is null or dac_family = 'conficker'
	-- filter whitelisted
	-- wl_rank = 6
	-- dn_rank = 

	-- -- uniqueness:
	AND rn = 1
	-- AND RCODE=3
	-- rn_qr = 1
	-- rn_qr_rcode = 1
	-- rn_mac = 1
	-- rn_mac_qr = 1
	-- rn_mac_qr_rcode = 1
	
GROUP BY
	"window";