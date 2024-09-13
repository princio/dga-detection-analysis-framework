
-- ALTER TYPE public.mcfp_info RENAME TO mcfp_info_tmp

-- ALTER TABLE IF EXISTS public.pcap RENAME mcfp_info TO mcfp_info_coltmp;

-- CREATE TYPE public.mcfp_info AS
-- (
-- 	folder text,
-- 	id text,
-- 	type mcfp_type,
-- 	pcap text
-- );
-- ALTER TYPE public.mcfp_info OWNER TO postgres;
-- ALTER TABLE IF EXISTS public.pcap ADD COLUMN mcfp_info mcfp_info;

UPDATE PUBLIC.PCAP
SET
	MCFP_INFO.ID = (P2.MCFP_INFO_COLTMP).ID,
	MCFP_INFO.FOLDER = (P2.MCFP_INFO_COLTMP).FOLDER,
	MCFP_INFO.TYPE = (P2.MCFP_INFO_COLTMP).TYPE,
	MCFP_INFO.PCAP = (P2.MCFP_INFO_COLTMP).PCAP
FROM
	PUBLIC.PCAP P2
WHERE
	P2.ID = PUBLIC.PCAP.ID
;

-- UPDATE pcap set
-- 	mcfp_info.id=(string_to_array("name", '_'))[1],
-- 	mcfp_info.folder='CTU-Malware-Capture-Botnet-' || (string_to_array("name", '_'))[1],
-- 	mcfp_info.type = 'BOTNET',
-- 	mcfp_info.pcap = substring("name" FROM (position('_' IN "name") + 1))
-- 	where dataset='CTU-13' and infected is true;
-- UPDATE pcap set
-- 	mcfp_info.id=(string_to_array("name", '_'))[1],
-- 	mcfp_info.folder='CTU-Normal-' || (string_to_array("name", '_'))[1],
-- 	mcfp_info.type = 'NORMAL',
-- 	mcfp_info.pcap = substring("name" FROM (position('_' IN "name") + 1))
-- 	where dataset='CTU-13' and infected is false;


-- ALTER TABLE IF EXISTS public.pcap DROP COLUMN mcfp_info_tmp;
-- DROP TYPE IF EXISTS public.mcfp_info_tmp;
-- SELECT MCFP_info,name FROM public.pcap where infected is false;