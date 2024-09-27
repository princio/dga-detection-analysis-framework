
from math import ceil
import os
import pandas as pd

import requests
from requests import Response
from requests.auth import HTTPBasicAuth
from db import Database
from ratelimit import limits, sleep_and_retry

db = Database()

url = 'https://dgarchive.caad.fkie.fraunhofer.de/reverse'

auth = HTTPBasicAuth('lorenzo_principi', 'galleydrownquintupleoverwritepromenadedonor')

ONE_MINUTE = 60


def get_dfreq():
	df = pd.read_sql('''
	SELECT
		DN.ID,
		DN.DN,
		DN.BDN,
		DN_NN_ALL.EPS1 as EPS1,
		DN_NN_ALL.EPS2 as EPS2,
		DN_NN_ALL.EPS3 as EPS3,
		DN_NN_ALL.EPS4 as EPS4,
		W1.RANK_DN,
		W1.RANK_BDN
	FROM
		DN
		JOIN DN_NN_ALL ON DN_NN_ALL.DN_ID = DN.ID
		JOIN WHITELIST1 W1 ON DN.ID=W1.DN_ID
	ORDER BY RANK_BDN   
	''', db.engine)

	eps_mask = df.apply(lambda x: all([x[f'eps{e}'] > 0.4 for e in range(1,5)]), axis=1)
	whitelist_mask = (df['rank_dn'] >= 10000) | df['rank_dn'].isna()
	df_req = df[eps_mask & whitelist_mask]
	return df_req.sort_values(by='id')


@sleep_and_retry
@limits(calls=1, period=ONE_MINUTE)
def call_api(listnewline) -> Response:
    response = requests.post(url, listnewline, auth=auth)
    if response.status_code != 200:
        raise Exception('API response: {} \n {}'.format(response.status_code, response.text))
    return response

df = get_dfreq()
totreqs = ceil(df.shape[0]/120)

def _save(fname, suffix, s):
    with open(f'tmp/{fname}.{suffix}', 'w') as fp:
        fp.write(s)
        pass

responses = []
for i in range(totreqs):
    print(f'Request {i}/{totreqs}...', end='')

    df_slice = df.iloc[i*100:(i+1)*100]
    
    fname = f'{df_slice['id'].min()}_{df_slice['id'].max()}'

    if os.path.exists(f'tmp/{fname}.success'):
        print(f'tmp/{fname}.success exists')
        continue

    df_slice[['id','dn']].to_csv(f'tmp/{fname}.list.csv')
    try:
        response = call_api('\n'.join(df_slice['dn'].to_list()))
        _save(fname, 'success', response.text) # type: ignore
        print('success')
    except Exception as e:
        _save(fname, 'error', str(e))
        response = e
        print('error', e)
        pass
    responses.append([fname, response])
    pass