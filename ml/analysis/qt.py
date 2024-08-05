


import copy
import enum
import json
from pathlib import Path
from typing import Literal, Union, cast

import numpy as np
import pandas as pd
from pandasgui import show

from defs import Database, FetchConfig, pgshow
from slot2 import FetchField, FetchFieldMetric, sql_healthy_dataset



class QTRow(enum.StrEnum):
    slotnum1 = 'slotnum1'
    slotnum2 = 'slotnum2'
    slotnum_added = 'slotnum_existent'
    slotnum_existent = 'slotnum_existent'

    slotnum_dga_r_gt0 = 'r: pos'
    slotnum_dga_ok_gt0 = 'ok: pos'
    slotnum_dga_nx_gt0 = 'nx: pos'

    slotnum_dga_gr_gt0 = 'gr: pos'
    slotnum_dga_gok_gt0 = 'gok: pos'
    slotnum_dga_gnx_gt0 = 'gnx: pos'

    slotnum_dga_gr_pos_gt0 = 'gr_pos: pos'
    slotnum_dga_gnx_pos_gt0 = 'gnx_pos: pos'
    slotnum_dga_gok_pos_gt0 = 'gok_pos: pos'

    slotnum_dga_ok_min = 'gok_pos: dga-slotnum min'
    slotnum_dga_nx_min = 'gnx_pos: dga-slotnum min'
    slotnum_dga_ok_max = 'gok_pos: dga-slotnum max'
    slotnum_dga_nx_max = 'gnx_pos: dga-slotnum max'
    slotnum_dga_ok_mean = 'gok_pos: dga-slotnum mean'
    slotnum_dga_nx_mean = 'gnx_pos: dga-slotnum mean'
    slotnum_dga_ok_sum = 'gok_pos: dga-slotnum sum'
    slotnum_dga_nx_sum = 'gnx_pos: dga-slotnum sum'
    r = 'r'
    gr = 'gr'
    gr_missed_hours = 'gr_missed_hours'
    th_ok = 'th_ok'
    th_nx = 'th_nx'
    pass

class QTRowOverall(enum.StrEnum):
    min_1hr_sum_gok_pos_of_8hr = 'gok_pos: min 1hr sum of 8hr'
    max_1hr_sum_gok_pos_of_8hr = 'gok_pos: max 1hr sum of 8hr'
    sum_1hr_sum_gok_pos_of_8hr = 'gok_pos: sum 1hr sum of 8hr'
    min_1hr_sum_gnx_pos_of_8hr = 'gnx_pos: min 1hr sum of 8hr'
    max_1hr_sum_gnx_pos_of_8hr = 'gnx_pos: max 1hr sum of 8hr'
    sum_1hr_sum_gnx_pos_of_8hr = 'gnx_pos: sum 1hr sum of 8hr'

    @staticmethod
    def _(cm: Union[str, Literal['TN', 'FA', 'MA', 'TA']], l: Union[str, Literal['gok_pos', 'gnx_pos']]):
        return f'{l}: {cm}'

    pass

class QTRowCM(enum.StrEnum):
    # first_TA_of_8hr_overlap_with_ok = 'gok_pos: first TA of 8hr overlap'
    # first_TA_of_8hr_overlap_with_nx = 'gnx_pos: first TA of 8hr overlap'

    @staticmethod
    def _(cm: Union[str, Literal['A', 'TN', 'FA', 'MA', 'TA']], l: Union[str, Literal['gok_pos', 'gnx_pos']]):
        return f'{l}: {cm}'

    @staticmethod
    def _2(l: Union[str, Literal['gok_pos', 'gnx_pos']], m: str):
        return f'{l}: dga-slotnum {m}'

    @staticmethod
    def _first(m: str, l: Union[str, Literal['gok_pos', 'gnx_pos']]):
        return f'{l}: first {m}'

    pass


class QT:
    def __init__(self, dir: Path, db: Database):
        self.db = db
        self.dir = dir
        pass

    def get_df(self, config: FetchConfig):
        fpath = self.dir.joinpath(f'{config.__hash__()}.csv')
        if Path(fpath).exists():
            df = pd.read_csv(fpath, index_col=0)
        else:
            with self.db.engine.connect() as conn:
                df = pd.read_sql(sql_healthy_dataset(config), conn)
                df.to_csv(fpath)
            pass
        df.slotnum = df.slotnum.astype(int)
        # print(df.slotnum.shape[0], df.slotnum.max() - df.slotnum.min() + 1)
        return df
    

    def df_fill_missing_slotnum(self, df, slotnummin, slotnummax):
        empty_row = {}
        for e in df.columns:
            empty_row[f'{e}'] = [] if e.find('dnagg') >= 0 else 0

        def gen_empty_row(slotnum):
            empty_row['slotnum'] = slotnum
            empty_row['slotnum_added'] = 1
            return copy.deepcopy(empty_row)

        if 'slotnum_added' not in df.columns:
            df.insert(0, 'slotnum_added', 0)
        newrows = [ gen_empty_row(slotnum) for slotnum in range(slotnummin, slotnummax) if slotnum not in df.slotnum.to_list() ]
        df = (df if len(newrows) == 0 else pd.concat([df, pd.DataFrame.from_records(newrows)]))
        return df[df.slotnum.between(slotnummin, slotnummax, inclusive='left')].reset_index(drop=True)
    
    def compare(self, config: FetchConfig, pcap_id: int, step: int = 1, renew = True, th_s=1.):
        DFH = self.get_df(config.new_fromsource(None))
        DFDGA = self.get_df(config.new_fromsource(pcap_id).new_offset(0))
        
        time_s_first = pd.read_sql("SELECT TIME_S FROM MESSAGE_%s WHERE FN=0" % pcap_id, self.db.engine).astype(np.float64).loc[0, 'time_s']
        time_s_first=cast(float, time_s_first)

        ls = [ 'ok' , 'nx' ]
        records = []
        dfh = DFH.iloc[0:8].reset_index(drop=True)
        dfh['slotnum_added'] = 0


        try:
            th = { f'{l}': dfh[FetchFieldMetric.g_pos(l)].max() * th_s for l in ls }
        except:
            print(dfh)
            print(dfh.columns)
            print(config.new_fromsource(pcap_id).new_offset(0).__hash__())
            exit(0)
        for i in range(0, DFDGA.slotnum.max() - 8, step):
            if i % 100:
                print(i, DFDGA.slotnum.max() - 8, sep='/')
            if renew:
                dfdga = self.get_df(config.new_fromsource(pcap_id).new_offset(time_s_first + i * 3600))
                # print(dfdga.slotnum.iloc[7] - dfdga.slotnum.iloc[0], end='\t')
                dfdga = self.df_fill_missing_slotnum(dfdga, i, i + 8)
                # print(dfdga.slotnum_added.sum())
            else:
                dfdga = self.df_fill_missing_slotnum(DFDGA, i, i + 8)
                pass

            try:
                dfsum = dfh[[e for e in FetchFieldMetric]] + dfdga[[e for e in FetchFieldMetric]]
            except:
                print(dfdga)
                print(dfdga.columns)
                print(config.new_fromsource(pcap_id).new_offset(0).__hash__())
                exit(0)
            
            overlap = {
                'slotnum1': dfdga.slotnum.min(),
                'slotnum2': dfdga.slotnum.max(),
                'slotnum_added': dfdga.slotnum_added.sum(),
                'slotnum_existent': 8 - dfdga.slotnum_added.sum(),
                'overlap translation step': step,
                'r': dfdga[FetchFieldMetric.r].sum(),
                'gr': dfdga[FetchFieldMetric.gr].sum(),
                'gr_missed_hours': dfdga[FetchFieldMetric.gr][(dfdga[FetchFieldMetric.g_pos('ok')].between(0, th['ok'], inclusive='both'))].sum(),
            }
            for l in ls:
                overlap[f'th_{l}'] = th[l]
                pass
            overlap[QTRowCM._('A', 'gr')] = (dfdga[FetchFieldMetric.gr] > 0).sum()
            overlap[QTRowCM._('A', 'gok')] = (dfdga[FetchFieldMetric.gok] > 0).sum()
            overlap[QTRowCM._('A', 'gnx')] = (dfdga[FetchFieldMetric.gnx] > 0).sum()
            overlap[QTRowCM._('A', FetchFieldMetric.g_pos('r'))] = (dfdga[FetchFieldMetric.g_pos('r')] > 0).sum()
            overlap[QTRowCM._('A', FetchFieldMetric.g_pos('ok'))] = (dfdga[FetchFieldMetric.g_pos('ok')] > 0).sum()
            overlap[QTRowCM._('A', FetchFieldMetric.g_pos('nx'))] = (dfdga[FetchFieldMetric.g_pos('nx')] > 0).sum()
            for l in ls:
                poscol = FetchFieldMetric.g_pos(l)
                overlap[QTRowCM._2(poscol, 'min')] = dfdga[poscol].min()
                overlap[QTRowCM._2(poscol, 'max')] = dfdga[poscol].max()
                overlap[QTRowCM._2(poscol, 'mean')] = dfdga[poscol].mean()
                overlap[QTRowCM._2(poscol, 'sum')] = dfdga[poscol].sum()
                overlap[f'{poscol}: pos'] = (dfdga[poscol] > 0).sum()
                A = dfdga[poscol] > 0
                P = dfsum[poscol] > th[l]
                TA = (A & P)
                MA = (A & ~P)
                FA = (~A & P)
                TN = (~(A | P))
                FIRST_TA = np.nan if (TA.sum() == 0) else (TA.idxmax() + 1)
                FIRST_MA = np.nan if (MA.sum() == 0) else (MA.idxmax() + 1)
                if (TA.sum() + MA.sum() + FA.sum() + TN.sum()) != 8:
                    df = pd.DataFrame.from_dict({'A': A, 'P': P})
                    df['TP'] = df.A & df.P
                    df['FP'] = (~df.A & df.P)
                    df['FN'] = (df.A & ~df.P)
                    df['TN'] = ~(df.A | df.P)
                    raise Exception(f'{TA} + {MA} + {FA} + {TN} < 8')
                overlap[f'{poscol}: ADay'] = A.sum() > 0 # type: ignore
                overlap[f'{poscol}: PDay'] = TA.sum() > 0 # type: ignore
                overlap[QTRowCM._first('TA', poscol)] = FIRST_TA
                overlap[QTRowCM._first('MA', poscol)] = FIRST_MA
                overlap[QTRowCM._('TA', poscol)] = TA.sum()
                overlap[QTRowCM._('MA', poscol)] = MA.sum()
                overlap[QTRowCM._('FA', poscol)] = FA.sum()
                overlap[QTRowCM._('TN', poscol)] = TN.sum()
                for agg in ['min','max','sum']:
                    overlap[f'{poscol}: {agg}'] = dfsum[poscol].agg(agg)
                pass
            # for l in ls:
            #     dnagg_pos_missed = dfdga[(dfdga[FetchField.g_pos(l)].between(0, th[l], inclusive='both'))][FetchField.g_pos_dnagg(l)]
            #     s = dnagg_pos_missed.apply(lambda x: json.loads(x.replace("'",'"')) if type(x) == str else x).explode().dropna().sort_values()
            #     dnagg_pos_missed = s.to_list()
            #     record[f'{l} dn missed count'] = len(dnagg_pos_missed)
            #     record[f'{l} dn missed'] = '  '.join(dnagg_pos_missed) # type: ignore
            #     pass

            records.append(overlap)
            pass

        df = pd.DataFrame.from_records(records)

        return df
    