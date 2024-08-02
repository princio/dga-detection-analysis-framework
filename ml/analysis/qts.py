

from pathlib import Path
from typing import List, Tuple
import warnings

import pandas as pd
import dtale

from defs import NN, FetchConfig
from qt import QTRow, QTRowCM, QT
from slot2 import FetchFieldMetric

def qt_pcaps(qt: QT, pcaps: List[Tuple[str, int]], fetch_config: FetchConfig, step = 8, renews = [False, True], th_s=1.):
    dfs = []

    for mw_name, pcap_id in pcaps:
        for renew in renews:
            df = qt.compare(fetch_config, pcap_id, renew=renew, step=step, th_s=th_s)
            dfs += [(f'df {pcap_id} {mw_name} {'' if renew else 'no-'}renew', df)]
            pass
        pass

    for df in dfs:
        df[1].insert(0, 'gb', 0)

    aggs = {}
    aggs[QTRowCM._('A', 'gr')] = 'sum'
    aggs[QTRowCM._('A', 'gnx')] = 'sum'
    aggs[QTRowCM._('A', 'gok')] = 'sum'
    aggs[QTRowCM._('A', FetchFieldMetric.g_pos1('r'))] = 'sum'
    aggs[QTRowCM._('A', FetchFieldMetric.g_pos1('ok'))] = 'sum'
    aggs[QTRowCM._('A', FetchFieldMetric.g_pos1('nx'))] = 'sum'
    for e in ['gnx_pos1', 'gok_pos1']:
        for e2 in ['TA', 'MA']:
            aggs[QTRowCM._(e2, e)] = [ 'sum' ]#'min', 'max', 'mean']
            aggs['first ' + QTRowCM._(e2, e)] = [ 'min', 'max', 'mean' ]#'min', 'max', 'mean']
        aggs[f'{e}: whole-day to-be-detected'] = 'sum'
        aggs[f'{e}: whole-day missed-detection'] = 'sum'
    grouped = {
        f'{a}': b.groupby(by='gb').aggregate(aggs).iloc[0] for a,b in dfs
    }

    dfs = [ (df[0], df[1].drop(columns='gb')) for df in dfs ]

    for i, df in enumerate(dfs[:-1]):
        dfs[i][1].index.names = ['metric']
        for l in [FetchFieldMetric.g_pos1('ok'), FetchFieldMetric.g_pos1('nx')]:
            dfs[i][1][QTRowCM._('TA', l)] = dfs[i][1][QTRowCM._('TA', l)] / (dfs[i][1][QTRowCM._('MA', l)] + dfs[i][1][QTRowCM._('TA', l)])
            dfs[i][1][QTRowCM._('MA', l)] = dfs[i][1][QTRowCM._('MA', l)] / (dfs[i][1][QTRowCM._('MA', l)] + dfs[i][1][QTRowCM._('TA', l)])

    df_overall = pd.DataFrame.from_records(grouped).T

    print(df_overall.index)
    print(df_overall)

    for e in [FetchFieldMetric.g_pos1('ok'), FetchFieldMetric.g_pos1('nx')]:
        for a in ['TA', 'MA']:
            df_overall[(QTRowCM._(f'{a}R', e), 'sum')] = df_overall[QTRowCM._(a, e)] / df_overall[QTRowCM._('A', e)]

    dfs += [ ('overall', df_overall.T) ]
    print(dfs[-1][1])
    dfs[-1][1].index.names = ['metric', 'how']
    for a, df in enumerate(dfs):
        df[1].to_csv(f'{df[0]}.csv')
        # d = dtale.show(df[1], hostname=df[0], subprocess=False,)


    input()

    # with warnings.catch_warnings():
    #     warnings.simplefilter('ignore')
    #     pandasgui.show(*dfs)
    #     pass
    pass