import itertools
from typing import Optional

from defs import MWTYPE, NN, OnlyFirsts, PacketType, SlotConfig



def groupby(c: SlotConfig, bydga):
    gb = 'GROUP BY SLOTNUM'
    if bydga:
        gb += ', PCAP_DGA, M.PCAP_MALWARE_TYPE, M.PCAP_INFECTED' if c.pcap_id is None else ''
    else:
        gb += ', PCAP_ID, PCAP_DGA, M.PCAP_MALWARE_TYPE, M.PCAP_INFECTED' if c.pcap_id is None else ''
    return gb

# def cw(glob=True, qr='qr', rcode = None, eps=None, th=None):
def cw(glob, qr, rcode, eps, th):
    condition = []
    label = []
    if glob:
        label += ['g']
        if rcode:
            condition.append("M.RN_QR_RCODE = 1")
        elif qr == 'q':
            condition.append("M.RN_QR = 1")
        elif qr == 'qr':
            condition.append("M.RN = 1")
            
    if rcode:
        condition.append(f"M.IS_R AND M.RCODE = {rcode}")
    elif qr == 'q':
        condition.append("NOT M.IS_R")
    elif qr == 'r':
        condition.append("M.IS_R")
    elif qr == 'qr':
        condition.append("TRUE")
    
    if rcode:
        label += ['NX' if rcode == 3 else ('OK' if rcode == 0 else rcode)]
    elif qr == 'q':
        label += ['Q']
    elif qr == 'r':
        label += ['R']
    elif qr == 'qr':
        label += ['QR']

    if eps:
        label += [f'POS_{eps}']
        condition.append(f"EPS{eps} > {th}")

    return f"SUM(CASE WHEN ({'AND'.join(condition)}) THEN 1 ELSE 0 END) as {'_'.join(label)}"

def sql_healthy_dataset(c: SlotConfig, slotnum: Optional[int] = None, mwtype: Optional[MWTYPE] = None, bydga: bool = False):
    def nnjoin(s, sep=",\n\t"):
        return sep.join([ s.format(i=i) for i in range(1,5) ])
    
    glob = [True, False]
    qr=['qr','q','r']
    rcode=[None,0,3]
    eps = [None, NN.NONE, NN.TLD, NN.ICANN, NN.PRIVATE]

    combinations = list(itertools.product(glob, qr, rcode, eps))
    for combo in combinations:
        print(cw(*(list(combo) + [c.th])))

    slots = f"""
SELECT
	FLOOR(M.TIME_S_TRANSLATED / {c.sps}) AS SLOTNUM,
    0 as DGA,
    false as PCAP_infected,

	{'COUNT(DISTINCT PCAP_ID) AS PCAPS_COUNT,' if c.pcap_id is None else ''}
	{'ARRAY_AGG(DISTINCT PCAP_ID) AS PCAPS,' if c.pcap_id is None else ''}

	{ nnjoin(f'COUNT(DISTINCT CASE WHEN EPS{{i}} > {c.th} THEN M.PCAP_ID ELSE NULL END) AS PCAP_POS_{{i}}') },
	{ nnjoin(f'ARRAY_AGG(DISTINCT CASE WHEN EPS{{i}} > {c.th} THEN M.PCAP_ID ELSE NULL END) AS PCAPID_POS{{i}}') },
    
	COUNT(DISTINCT DN_ID) AS U,

    { cw(glob=False, qr='qr')}
	COUNT(*) AS QR,
	SUM(CASE WHEN NOT M.IS_R THEN 0 ELSE 1 END) AS Q,
	SUM(CASE WHEN M.IS_R THEN 1 ELSE 0 END) AS R,
	SUM(CASE WHEN M.IS_R AND M.RCODE = 0 THEN 1 ELSE 0 END) AS OK,
	SUM(CASE WHEN M.IS_R AND M.RCODE = 3 THEN 1 ELSE 0 END) AS NX,


	SUM(CASE WHEN M.RN = 1 THEN 1 ELSE 0 END) AS gQR,
	SUM(CASE WHEN M.RN_QR = 1 AND NOT M.IS_R THEN 1 ELSE 0 END) AS gQ,
	SUM(CASE WHEN M.RN_QR = 1 AND M.IS_R IS TRUE THEN 1 ELSE 0 END) AS gR,
	SUM(CASE WHEN M.RN_QR_RCODE = 1 AND M.IS_R IS TRUE AND M.RCODE=0 THEN 1 ELSE 0 END) AS gOK,
	SUM(CASE WHEN M.RN_QR_RCODE = 1 AND M.IS_R IS TRUE AND M.RCODE=3 THEN 1 ELSE 0 END) AS gNX,


	{ nnjoin(f'SUM(CASE WHEN EPS{{i}} > {c.th} AND M.RN = 1 THEN 1 ELSE 0 END) AS gQR_POS_{{i}}') },
	{ nnjoin(f'SUM(CASE WHEN EPS{{i}} > {c.th} AND M.RN_QR = 1 AND M.IS_R IS FALSE THEN 1 ELSE 0 END) AS gQ_POS_{{i}}') },
	{ nnjoin(f'SUM(CASE WHEN EPS{{i}} > {c.th} AND M.RN_QR = 1 AND M.IS_R IS FALSE THEN 1 ELSE 0 END) AS gQ_POS_{{i}}') },
	{ nnjoin(f'SUM(CASE WHEN EPS{{i}} > {c.th} AND M.RN_QR = 1 AND M.IS_R IS TRUE THEN 1 ELSE 0 END) AS gR_POS_{{i}}') },
	{ nnjoin(f'SUM(CASE WHEN EPS{{i}} > {c.th} AND M.RN_QR_RCODE = 1 AND M.IS_R IS TRUE AND M.RCODE=0 THEN 1 ELSE 0 END) AS gOK_POS_{{i}}') },
	{ nnjoin(f'SUM(CASE WHEN EPS{{i}} > {c.th} AND M.RN_QR_RCODE = 1 AND M.IS_R IS TRUE AND M.RCODE=3 THEN 1 ELSE 0 END) AS gNX_POS_{{i}}') },


	{ nnjoin(f'SUM(CASE WHEN EPS{{i}} > {c.th} AND M.RN = 1 THEN 1 ELSE 0 END) AS gQR_POS_{{i}}') },
	{ nnjoin(f'SUM(CASE WHEN EPS{{i}} > {c.th} AND M.RN_QR = 1 AND M.IS_R IS FALSE THEN 1 ELSE 0 END) AS gQ_POS_{{i}}') },
	{ nnjoin(f'SUM(CASE WHEN EPS{{i}} > {c.th} AND M.RN_QR = 1 AND M.IS_R IS FALSE THEN 1 ELSE 0 END) AS gQ_POS_{{i}}') },
	{ nnjoin(f'SUM(CASE WHEN EPS{{i}} > {c.th} AND M.RN_QR = 1 AND M.IS_R IS TRUE THEN 1 ELSE 0 END) AS gR_POS_{{i}}') },
	{ nnjoin(f'SUM(CASE WHEN EPS{{i}} > {c.th} AND M.RN_QR_RCODE = 1 AND M.IS_R IS TRUE AND M.RCODE=0 THEN 1 ELSE 0 END) AS gOK_POS_{{i}}') },
	{ nnjoin(f'SUM(CASE WHEN EPS{{i}} > {c.th} AND M.RN_QR_RCODE = 1 AND M.IS_R IS TRUE AND M.RCODE=3 THEN 1 ELSE 0 END) AS gNX_POS_{{i}}') },



	{ nnjoin(f'ARRAY_REMOVE(ARRAY_AGG(CASE WHEN EPS{{i}} > {c.th} THEN DN ELSE NULL END), NULL) AS POS_{{i}}_DNAGG') }
FROM
	get_message_healthy_all2() AS M

    {'JOIN PCAP_MALWARE PMW ON M.PCAP_ID=PMW.ID' if c.pcap_id is None else ''}

    {groupby(c, bydga)}
"""
    

    with open("tmp/sql2.txt","w") as fp:
        fp.write(f"""
WITH SLOTS AS ({slots}) SELECT * FROM SLOTS 
""")
    return f"""
WITH SLOTS AS ({slots}) SELECT * FROM SLOTS 
"""
