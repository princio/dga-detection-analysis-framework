from typing import Optional

from defs import MWTYPE, OnlyFirsts, PacketType, SlotConfig



def groupby(c: SlotConfig, bydga):
    gb = 'GROUP BY SLOTNUM'
    if bydga:
        gb += ', PCAP_DGA, M.PCAP_MALWARE_TYPE, M.PCAP_INFECTED' if c.pcap_id is None else ''
    else:
        gb += ', PCAP_ID, PCAP_DGA, M.PCAP_MALWARE_TYPE, M.PCAP_INFECTED' if c.pcap_id is None else ''
    return gb

def build_sql_pcap(config: SlotConfig):
    where = {}
    where[PacketType.QR.value.lower()] = ""
    where[PacketType.Q.value.lower()] = " WHERE m.IS_R IS FALSE "
    where[PacketType.R.value.lower()] = " WHERE m.IS_R IS TRUE "
    where[PacketType.NX.value.lower()] = " WHERE m.RCODE=3 "
    where = where[config.packet_type]

    return f"""
(
    WITH whitelist AS (
         SELECT whitelist_dn.id,
            whitelist_dn.dn_id,
            whitelist_dn.whitelist_id,
            whitelist_dn.rank_dn,
            whitelist_dn.rank_bdn
           FROM whitelist_dn
          WHERE whitelist_dn.whitelist_id = 1
        ),
        message_dataset AS (
         SELECT m.id,
            m.pcap_id,
            m.time_s,
            m.fn,
            m.fn_req,
            m.dn_id,
            m.qcode,
            m.is_r,
            m.rcode,
            m.server,
            m.answer,
            m.time_s_translated,
            m.tmp_min_time,
            m.pcap_infected,
            m.pcap_malware_type,
            m.time_s_plus100ms
           FROM message_{config.pcap_id} m
           {where}
        ), message_dataset_rownumber AS (
         SELECT message_dataset.id,
            message_dataset.pcap_id,
            message_dataset.time_s,
            message_dataset.fn,
            message_dataset.fn_req,
            message_dataset.dn_id,
            message_dataset.qcode,
            message_dataset.is_r,
            message_dataset.rcode,
            message_dataset.server,
            message_dataset.answer,
            message_dataset.time_s_translated,
            message_dataset.tmp_min_time,
            message_dataset.pcap_infected,
            message_dataset.pcap_malware_type,
            message_dataset.time_s_plus100ms,
            row_number() OVER (PARTITION BY message_dataset.dn_id ORDER BY message_dataset.time_s_translated) AS __rn
           FROM message_dataset
        )
 SELECT mdr.id,
    mdr.pcap_id,
    mdr.time_s,
    mdr.fn,
    mdr.fn_req,
    mdr.dn_id,
    mdr.qcode,
    mdr.is_r,
    mdr.rcode,
    mdr.server,
    mdr.answer,
    mdr.time_s_translated,
    mdr.tmp_min_time,
    mdr.pcap_infected,
    mdr.pcap_malware_type,
    mdr.time_s_plus100ms,
    mdr.__rn,
    dn.dn,
    dn.bdn,
    dn_nn_all.eps1,
    dn_nn_all.eps2,
    dn_nn_all.eps3,
    dn_nn_all.eps4,
    whitelist.rank_dn,
    whitelist.rank_bdn
   FROM message_dataset_rownumber mdr
     JOIN dn ON mdr.dn_id = dn.id
     JOIN dn_nn_all ON dn_nn_all.dn_id = mdr.dn_id
     JOIN whitelist ON whitelist.dn_id = mdr.dn_id
  {'WHERE mdr.__rn = 1' if config.onlyfirsts != OnlyFirsts.ALL.value.lower() else ''}
  ORDER BY mdr.time_s_translated
)
"""

def build_sql(c: SlotConfig, slotnum: Optional[int] = None, mwtype: Optional[MWTYPE] = None, bydga: bool = False):
    def nnjoin(s, sep=",\n\t"):
        return sep.join([ s.format(i=i) for i in range(1,5) ])
        
    if c.pcap_id:
        # table_from = build_sql_pcap(c)
        # with open("tmp/sql.txt","w") as fp:
        #     fp.write(table_from)
        table_from = f"(select * from get_message_dataset({c.pcap_id}))"
    else:
        table_from = f"{c.packet_type}_{c.onlyfirsts}"
    
    final_where = []
    if slotnum is not None:
        final_where.append("SLOTNUM = %d" % (slotnum))
        pass
    if mwtype:
        final_where.append("DGA=%s" % (mwtype))
        pass
    if len(final_where):
        final_where = "WHERE " + " AND ".join(final_where)
    else:
        final_where = ""
        
    slots = f"""
SELECT
	FLOOR(M.TIME_S_TRANSLATED / {c.sps}) AS SLOTNUM,

	{'m.PCAP_ID AS PCAP_ID,' if not bydga else ''}
	{'PMW.DGA AS PCAP_DGA,' if c.pcap_id is None else ''}
    {"array_position(ARRAY['no-malware', 'non-dga', 'dga']::malware_type[], M.pcap_malware_type) - 1 AS DGA," if c.pcap_id is None else ''}
    {'M.PCAP_infected,' if c.pcap_id is None else ''}

	{'COUNT(DISTINCT PCAP_ID) AS PCAPS_COUNT,' if c.pcap_id is None else ''}
	{'ARRAY_AGG(DISTINCT PCAP_ID) AS PCAPS,' if c.pcap_id is None else ''}

	SUM(
		CASE
			WHEN RANK_BDN > {c.wl_th} THEN 1
			ELSE 0
		END
	) AS RANK_BDN,

	{ nnjoin(f'COUNT(DISTINCT CASE WHEN EPS{{i}} > {c.th} THEN M.PCAP_ID ELSE NULL END) AS PCAP_POS_{{i}}') },
	{ nnjoin(f'ARRAY_AGG(DISTINCT CASE WHEN EPS{{i}} > {c.th} THEN M.PCAP_ID ELSE NULL END) AS PCAPID_POS{{i}}') },
    
	COUNT(*) AS TOTAL,
	SUM(CASE WHEN M.IS_R THEN 0 ELSE 1 END) AS Q,
	SUM(CASE WHEN M.IS_R THEN 1 ELSE 0 END) AS R,
	COUNT(DISTINCT DN_ID) AS U,
	{ nnjoin(f'SUM(CASE WHEN EPS{{i}} > {c.th} THEN 1 ELSE NULL END) AS POS_{{i}}') },

    -- add materialized view with just queries, nx, responses with first_by_(pcap/global).

	{ nnjoin(f'SUM(CASE WHEN RANK_{c.wl_col} >  {c.wl_th} AND EPS{{i}} >  {c.th} THEN 1 ELSE 0 END) AS POS_{{i}}_WL') },

	{ nnjoin(f'ARRAY_REMOVE(ARRAY_AGG(CASE WHEN EPS{{i}} > {c.th} THEN DN ELSE NULL END), NULL) AS POS_{{i}}_DNAGG') }
FROM
	{table_from} AS M

    {'JOIN PCAP_MALWARE PMW ON M.PCAP_ID=PMW.ID' if c.pcap_id is None else ''}

    {groupby(c, bydga)}
"""
    

    with open("tmp/sql2.txt","w") as fp:
        fp.write(f"""
WITH SLOTS AS ({slots}) SELECT * FROM SLOTS {final_where}
""")
    return f"""
WITH SLOTS AS ({slots}) SELECT * FROM SLOTS {final_where}
"""
