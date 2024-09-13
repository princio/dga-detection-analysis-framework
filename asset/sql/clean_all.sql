
drop table IF EXISTS message_1;
drop table IF EXISTS message_2;

delete from malware where true;
ALTER SEQUENCE malware_id_seq RESTART WITH 1;
INSERT INTO public.malware(name, dga) VALUES ('no-infection', 0);

delete from whitelist_dn where true;
ALTER SEQUENCE whitelist_dn_id_seq RESTART WITH 1;

delete from dn_nn where true;
ALTER SEQUENCE dn_nn_id_seq RESTART WITH 1;

delete from dn where true;
ALTER SEQUENCE dn_id_seq RESTART WITH 1;

delete from pcap where true;
ALTER SEQUENCE pcap_id_seq RESTART WITH 1;

ALTER SEQUENCE message_id_seq RESTART WITH 1;


delete from whitelist_list where true;
ALTER SEQUENCE whitelist_list_id_seq RESTART WITH 1;

delete from whitelist where true;
ALTER SEQUENCE whitelist_id_seq RESTART WITH 1;



-- delete from nn where true;
-- ALTER SEQUENCE nn_id_seq RESTART WITH 1;
