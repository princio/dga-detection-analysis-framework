

from pathlib import Path
from typing import List, Tuple
import warnings

import numpy as np
import pandas as pd

from defs import NN, FetchConfig, pgshow
from qt import QTRow, QTRowCM, QT
from slot2 import FetchFieldMetric

def qt_pcaps(qt: QT, pcaps: List[Tuple[str, int]], fetch_config: FetchConfig, step = 8, renews = [False], th_s=1.):
    dfs = []

    n = len(pcaps) + len(renews) + 2
    i = 1
    for mw_name, pcap_id in pcaps:
        if mw_name != 'caphaw':
            for renew in renews:
                for th in [1., 1.5]:
                    print(f'{i}/{n}')
                    df = qt.compare(fetch_config, pcap_id, renew=renew, step=step, th_s=th)
                    dfs += [(f'df {pcap_id} {mw_name} {'' if renew else 'no-'}renew th_{th}', df)]
                    i += 1
            pass
        pass

    for df in dfs:
        df[1].insert(0, 'gb', 0)

    aggs = {}
    aggs[QTRowCM._('A', 'gr')] = 'sum'
    aggs[QTRowCM._('A', 'gnx')] = 'sum'
    aggs[QTRowCM._('A', 'gok')] = 'sum'
    aggs[QTRowCM._('A', FetchFieldMetric.g_pos('r'))] = 'sum'
    aggs[QTRowCM._('A', FetchFieldMetric.g_pos('ok'))] = 'sum'
    aggs[QTRowCM._('A', FetchFieldMetric.g_pos('nx'))] = 'sum'
    for e in ['gnx_pos', 'gok_pos']:
        for e2 in ['TA', 'MA']:
            def _first(x):
                return x.min(skipna=True)
            aggs[QTRowCM._(e2, e)] = [ 'sum' ]#'min', 'max', 'mean']
            aggs[QTRowCM._first(e2, e)] = [
                # 'min',
                # 'max',
                'mean'
            ]#'min', 'max', 'mean']
        aggs[f'{e}: ADay'] = 'sum'
        aggs[f'{e}: PDay'] = 'sum'
    grouped = {
        f'{a}': b.groupby(by='gb').aggregate(aggs).iloc[0] for a,b in dfs
    }

    dfs = [ (df[0], df[1].drop(columns='gb')) for df in dfs ]

    df_overall = pd.DataFrame.from_records(grouped).T

    for col in df_overall.columns:
        print(col)

    for e in [FetchFieldMetric.g_pos('ok'), FetchFieldMetric.g_pos('nx')]:
        for a in ['TA', 'MA']:
            df_overall[(QTRowCM._(f'{a}R', e), 'ratio')] = df_overall[(QTRowCM._(a, e), 'sum')] / df_overall[(QTRowCM._('A', e), 'sum')]
        df_overall[(QTRowCM._(f'TDR', e), 'ratio')] = df_overall[(QTRowCM._('ADay', e), 'sum')] / (df_overall[(QTRowCM._('ADay', e), 'sum')] + df_overall[(QTRowCM._('PDay', e), 'sum')])

    print(df_overall)

    df_overall.columns = pd.MultiIndex.from_tuples([tuple(i[0].split(': ') + [i[1]]) for i in df_overall.columns], names=['nn', 'metric', 'how'])
    df_overall = df_overall[[col for col in df_overall.columns if col[0] in ['gok_pos', 'gnx_pos']]]
    df_overall = df_overall.T
    dfs += [ ('overall', df_overall) ]

    df_overall = df_overall.unstack(0)
    # df_overall = df_overall.map(lambda x: np.nan if np.isnan(x) else (f'{int(x)}' if x == int(x) else f'{round(x, 2)}') )
    df_overall.to_csv(f'overall_{fetch_config.nn}.csv')

    print(df_overall)

    # dfs[-1][1].index.names = ['metric', 'how']
    for a, df in enumerate(dfs):
        df[1].to_csv(f'{df[0]}.csv')
        # d = dtale.show(df[1], hostname=df[0], subprocess=False,)


    # input()

    # with warnings.catch_warnings():
    #     warnings.simplefilter('ignore')
    #     pandasgui.show(*dfs)
    #     pass
    pass