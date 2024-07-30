import itertools
from typing import Optional, Union

import pandas as pd

from defs import NN, FetchConfig



# def cw(glob=True, qr='qr', rcode = None, eps=None, th=None):
def cw(eps, glob, pt, pn, th):
    condition = []
    label = []
            
    if pt == 'nx':
        if glob:
            condition.append("M.RN_QR_RCODE = 1")
        condition.append(f"M.IS_R AND M.RCODE = 3")
    elif pt == 'ok':
        if glob:
            condition.append("M.RN_QR_RCODE = 1")
        condition.append(f"M.IS_R AND M.RCODE = 0")
    elif pt == 'q':
        if glob:
            condition.append("M.RN_QR = 1")
        condition.append("NOT M.IS_R")
    elif pt == 'r':
        if glob:
            condition.append("M.RN_QR = 1")
        condition.append("M.IS_R")
    elif pt == 'qr':
        if glob:
            condition.append("M.RN = 1")
        condition.append("TRUE")
    else:
        raise Exception('PT must be defined.')
    
    label.append(f"{'g' if glob else ''}{pt.upper()}")

    if eps:
        if pn == 'pos':
            label.append(f'POS{eps}')
            condition.append(f"EPS{eps} > {th}")
        elif pn == 'neg':
            label.append(f'NEG{eps}')
            condition.append(f"EPS{eps} <= {th}")
        else:
            raise Exception('PN must be defined.')
        pass

    condition = ' AND '.join(condition)
    label = '_'.join(label)

    return label, condition

def cw_sum(eps, glob, pt, pn, th):
    label, condition = cw(eps, glob, pt, pn, th)
    return f"SUM(CASE WHEN ({condition}) THEN 1 ELSE 0 END) as {label}"

def cw_agg(eps, glob, pt, pn, th):
    label, condition = cw(eps, glob, pt, pn, th)
    return f'ARRAY_REMOVE(ARRAY_AGG(CASE WHEN ({condition}) THEN DN ELSE NULL END), NULL) AS {label}_DNAGG'

"""
Return a sql that will fetch:
    slotnum,
    pcap_infected, pcaps_count, pcaps, pcap_pos_1, pcapid_pos1,
    u, qr, q, r, nx, ok,
    gqr, gq, gr, gnx, gok,
    qr_pos1, q_pos1, r_pos1, nx_pos1, ok_pos1,
    gqr_pos1, gq_pos1, gr_pos1, gnx_pos1, gok_pos1,
    gnx_neg1_dnagg, gnx_pos1_dnagg, gok_neg1_dnagg, gok_pos1_dnagg
where 1 could be 1,2,3,4.
"""
def sql_healthy_dataset(config: FetchConfig):
    sps = config.sps
    nn = config.nn
    th = config.th
    pcap_id = config.pcap_id
    pcap_offset = 0

    if pcap_id is None:
        table_from = 'get_message_healthy_all2()'
    else:
        table_from = f'get_message_pcap_all3({pcap_id}, {pcap_offset})'
    
    cws_sum = [cw_sum(*(list(combo) + [th])) for combo in list(itertools.product([False, nn], [False, True], ['qr','q','r','nx','ok'], ['pos']))]
    cws_agg = [cw_agg(*(list(combo) + [th])) for combo in list(itertools.product([nn], [True], ['nx','ok'], ['neg', 'pos']))]

    slots = f"""
SELECT
	FLOOR(M.TIME_S_TRANSLATED / {sps}) AS SLOTNUM,

	COUNT(DISTINCT PCAP_ID) AS PCAPS_COUNT,
	ARRAY_AGG(DISTINCT PCAP_ID) AS PCAPS,

	{ f'COUNT(DISTINCT CASE WHEN EPS{nn} > {th} THEN M.PCAP_ID ELSE NULL END) AS PCAP_POS{nn}' },
	{ f'ARRAY_AGG(DISTINCT CASE WHEN EPS{nn} > {th} THEN M.PCAP_ID ELSE NULL END) AS PCAPID_POS{nn}' },
    
	COUNT(DISTINCT DN_ID) AS U,

    { ',\n\t'.join(cws_sum) },
    { ',\n\t'.join(cws_agg) }
    
FROM
	{table_from} AS M

    GROUP BY SLOTNUM
"""
    
    with open("tmp/sql2.txt","w") as fp:
        fp.write(f"""
WITH SLOTS AS ({slots}) SELECT * FROM SLOTS 
""")
    return f"""
WITH SLOTS AS ({slots}) SELECT * FROM SLOTS 
"""
