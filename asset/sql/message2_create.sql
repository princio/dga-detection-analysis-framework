-- Table: public.message2

-- DROP TABLE IF EXISTS public.message2;

CREATE TABLE IF NOT EXISTS public.message2
(
    id bigint NOT NULL GENERATED ALWAYS AS IDENTITY ( INCREMENT 1 START 1 MINVALUE 1 MAXVALUE 9223372036854775807 CACHE 1 ),
    pcap_id bigint NOT NULL,
    time_s timestamp without time zone NOT NULL,
    fn bigint NOT NULL,
    fn_req bigint NOT NULL,
    dn_id bigint NOT NULL,
    qcode integer NOT NULL,
    is_r boolean NOT NULL,
    rcode integer,
    src text COLLATE pg_catalog."default",
    dst text COLLATE pg_catalog."default",
    answer text COLLATE pg_catalog."default",
    seconds double precision,
    CONSTRAINT message2_pkey PRIMARY KEY (id, pcap_id)
) PARTITION BY LIST (pcap_id);

ALTER TABLE IF EXISTS public.message2
    OWNER to postgres;

COMMENT ON COLUMN public.message2.seconds
    IS 'Seconds elapsed since the first message of the day.';
-- Index: message2_dn_id_is_r_rcode_idx

-- DROP INDEX IF EXISTS public.message2_dn_id_is_r_rcode_idx;

CREATE INDEX IF NOT EXISTS message2_dn_id_is_r_rcode_idx
    ON public.message2 USING btree
    (dn_id ASC NULLS LAST)
    INCLUDE(is_r, rcode)
    WITH (deduplicate_items=True)
;

-- Partitions SQL

CREATE TABLE public.message2_it2016_0 PARTITION OF public.message2
    FOR VALUES IN ('672', '673', '674', '675', '676', '677', '678', '679', '680', '681', '682', '683', '684', '685', '686', '687', '688', '689', '690', '691', '692', '693', '694', '695')
TABLESPACE pg_default;

ALTER TABLE IF EXISTS public.message2_it2016_0
    OWNER to postgres;
CREATE TABLE public.message2_it2016_1 PARTITION OF public.message2
    FOR VALUES IN ('697', '698', '699', '700', '701', '702', '703', '704', '705', '706', '707', '708', '709', '710', '711', '712', '713', '714', '715', '716', '717', '718', '719', '720')
TABLESPACE pg_default;

ALTER TABLE IF EXISTS public.message2_it2016_1
    OWNER to postgres;
CREATE TABLE public.message2_it2016_2 PARTITION OF public.message2
    FOR VALUES IN ('724', '725', '726', '727', '728', '729', '730', '731', '732', '733', '734', '735', '736', '737', '738', '739', '740', '741', '742', '743', '744', '745', '746', '747')
TABLESPACE pg_default;

ALTER TABLE IF EXISTS public.message2_it2016_2
    OWNER to postgres;
CREATE TABLE public.message2_it2016_3 PARTITION OF public.message2
    FOR VALUES IN ('748', '749', '750', '751', '752', '753', '754', '755', '756', '757', '758', '759', '760', '761', '762', '763', '764', '765', '766', '767', '768', '769', '770', '771')
TABLESPACE pg_default;

ALTER TABLE IF EXISTS public.message2_it2016_3
    OWNER to postgres;
CREATE TABLE public.message2_it2016_4 PARTITION OF public.message2
    FOR VALUES IN ('772', '773', '774', '775', '776', '777', '778', '779', '780', '781', '782', '783', '784', '785', '786', '787', '788', '789', '790', '791', '792', '793', '794', '795')
TABLESPACE pg_default;

ALTER TABLE IF EXISTS public.message2_it2016_4
    OWNER to postgres;
CREATE TABLE public.message2_it2016_5 PARTITION OF public.message2
    FOR VALUES IN ('796', '797', '798', '799', '800', '801', '802', '803', '804', '805', '806', '807', '808', '809', '810', '811', '812', '813', '814', '815', '816', '817', '818', '819')
TABLESPACE pg_default;

ALTER TABLE IF EXISTS public.message2_it2016_5
    OWNER to postgres;
CREATE TABLE public.message2_it2016_6 PARTITION OF public.message2
    FOR VALUES IN ('820', '821', '822', '823', '824', '825', '826', '827', '828', '829', '830', '831', '832', '833', '834', '835', '836', '837', '838', '839', '840', '841', '842', '843')
TABLESPACE pg_default;

ALTER TABLE IF EXISTS public.message2_it2016_6
    OWNER to postgres;
CREATE TABLE public.message2_it2016_7 PARTITION OF public.message2
    FOR VALUES IN ('1050', '1051', '1052', '1053', '1054', '1055', '1056', '1057', '1058', '1059', '1060', '1061', '1062', '1063', '1064', '1065', '1066', '1067', '1068', '1069', '1070', '1071', '1072', '1073')
TABLESPACE pg_default;

ALTER TABLE IF EXISTS public.message2_it2016_7
    OWNER to postgres;
CREATE TABLE public.message2_it2016_8 PARTITION OF public.message2
    FOR VALUES IN ('1074', '1075', '1076', '1077', '1078', '1079', '1080', '1081', '1082', '1083', '1084', '1085', '1086', '1087', '1088', '1089', '1090', '1091', '1092', '1093', '1094', '1095', '1096', '1097')
TABLESPACE pg_default;

ALTER TABLE IF EXISTS public.message2_it2016_8
    OWNER to postgres;
CREATE TABLE public.message2_it2016_9 PARTITION OF public.message2
    FOR VALUES IN ('1098', '1099', '1100', '1101', '1102', '1103', '1104', '1105', '1106', '1107', '1108', '1109', '1110', '1111', '1112', '1113', '1114', '1115', '1116', '1117', '1118', '1119', '1120', '1121')
TABLESPACE pg_default;

ALTER TABLE IF EXISTS public.message2_it2016_9
    OWNER to postgres;