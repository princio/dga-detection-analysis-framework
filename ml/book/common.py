import enum
from os import name
import pickle
from typing import Dict, List, Union
import warnings
import pandas as pd
from dataclasses import dataclass
from mlxtend.feature_selection import SequentialFeatureSelector
import psycopg2
from sklearn.metrics import confusion_matrix, make_scorer
from sklearn.model_selection import RandomizedSearchCV, train_test_split, cross_validate, StratifiedKFold, cross_val_predict
import numpy as np
from sqlalchemy import create_engine
from typing import Optional, List


class DATASETS(enum.StrEnum):
    CTU13="CTU-13"
    CTUSME="CTU-SME-13"
    def __str__(self):
        return self.value
    def __repr(self):
        return self.__str__()

class WLFIELD(enum.StrEnum):
    DN="DN"
    BDN="BDN"
    def __str__(self):
        return self.value
    def __repr(self):
        return self.__str__()


class MWTYPE(enum.StrEnum):
    NIC="no-malware"
    NONDGA="non-dga"
    DGA="dga"
    def __str__(self):
        return self.value
    def __repr(self):
        return self.__str__()

@dataclass(frozen=True)
class SlotConfig:
    dataset: DATASETS = DATASETS.CTU13  # Sostituisci con il tipo corretto se diverso da stringa
    sps: int = 1 * 60 * 60
    onlyfirsts: Optional[str] = False
    th: float = 0.999
    wl_th: int = 10000

    nn: Optional[Union[int, List[int]]] = None
    wl_col: Optional[WLFIELD] = None

    def __hash__(self):
        return hash((self.sps, self.th, self.wl_th, self.dataset, self.onlyfirsts, self.nn, self.wl_col))

    def __eq__(self, other):
        if not isinstance(other, SlotConfig):
            return NotImplemented
        return (self.sps == other.sps and
                self.th == other.th and
                self.wl_th == other.wl_th and
                self.dataset == other.dataset and
                self.onlyfirsts == other.onlyfirsts and
                self.wl_col == other.wl_col)
    
    def __str__(self):
        return (f"SlotConfig: [SPS: {self.sps}, TH: {self.th}, WL_TH: {self.wl_th}, "
                f"DATASET: {self.dataset}, onlyfirsts: {self.onlyfirsts}, WL_COL: {self.wl_col}]")

    def __repr__(self):
        return self.__str__()


def build_sql(c: SlotConfig, slotnum: Optional[int] = None, mwtype: Optional[MWTYPE] = None, with_dn = False):

    def nnjoin(s, sep=",\n\t"):
        return sep.join([ s.format(i=i) for i in range(1,5) ])

    messages_select = []
    messages_where = []
    if c.onlyfirsts:
        if c.onlyfirsts == "pcap":
            messages_select.append("DISTINCT ON (M.DN_ID, M.PCAP_ID) M.*")
        elif c.onlyfirsts == "global":
            messages_select.append("DISTINCT ON (M.DN_ID) M.*")
        else:
            messages_select.append("M.*")
    else:
        messages_select.append("M.*")
    dn_agg_neg = ""
    dn_agg_pos = ""
    if with_dn:
        messages_select.append("DN.DN")
        dn_agg_neg = nnjoin(f'array_remove(array_agg(CASE WHEN DN_NN{{i}}.VALUE <= {c.th} THEN M.DN ELSE NULL END), NULL) AS DNAGG_NN{{i}}_NEG' )
        dn_agg_pos = nnjoin(f'array_remove(array_agg(CASE WHEN DN_NN{{i}}.VALUE > {c.th} THEN M.DN ELSE NULL END), NULL) AS DNAGG_NN{{i}}_POS' )
        dn_agg_neg = f"{dn_agg_neg},"
        dn_agg_pos = f"{dn_agg_pos},"
    messages_select.append("DN.BDN")
        
    if slotnum is not None:
        messages_where.append("FLOOR(TIME_S_TRANSLATED / (%d)) = %d" % (c.sps, slotnum))
        pass
    if mwtype:
        messages_where.append("(M.STATUS).MALWARE_TYPE='%s'" % (mwtype))
        pass
    if len(messages_where):
        messages_where = "WHERE " + " AND ".join(messages_where)
    else:
        messages_where = ""
        
    submessages = f"""
SELECT
    { ",".join(messages_select) }
FROM
    MESSAGE AS M JOIN DN ON M.DN_ID = DN.ID
    { messages_where }
"""
    sql = f"""
WITH
WHITELIST AS (SELECT * FROM WHITELIST_DN WHERE WHITELIST_ID = 1),
{ nnjoin('DN_NN{i} AS (SELECT * FROM DN_NN WHERE NN_ID = {i})') },
PCAPMW AS (SELECT PCAP.ID, DGA FROM PCAP JOIN MALWARE ON PCAP.MALWARE_ID = MALWARE.ID WHERE PCAP.DATASET='{c.dataset}'),
SUBMESSAGES AS ({submessages}),
MESSAGES AS (
  SELECT 
    __M.DN_ID,
    __M.PCAP_ID,
    __M.BDN,
    { '__M.DN,' if with_dn else '' }
    FLOOR(__M.TIME_S_TRANSLATED / ({c.sps})) AS SLOTNUM, 
    COUNT(*) AS QR,
    SUM(CASE WHEN __M.IS_R IS FALSE THEN 1 ELSE 0 END) AS Q,
    SUM(CASE WHEN __M.IS_R IS TRUE THEN 1 ELSE 0 END) AS R,
    SUM(CASE WHEN __M.IS_R IS TRUE AND RCODE = 3 THEN 1 ELSE 0 END ) AS NX
  FROM
      SUBMESSAGES __M
  GROUP BY
    __M.DN_ID,
    __M.PCAP_ID,
    __M.BDN,
    { '__M.DN,' if with_dn else '' }
    SLOTNUM
)
SELECT
  M.PCAP_ID,
  P.Q AS QQ,
  P.U AS UU,
  P.DURATION AS D,
  PCAPMW.DGA,
  M.SLOTNUM,
  SUM(CASE WHEN WL.RANK_BDN > 1000000 THEN 1 ELSE 0 END) AS RANK_BDN,
  COUNT(DISTINCT M.DN_ID) AS U,
  { dn_agg_neg }
  { dn_agg_pos }
  -- array_agg(DISTINCT CASE WHEN DN_NN1.VALUE > {c.th} THEN M.DN_ID ELSE NULL END) AS DNU,
  COUNT(DISTINCT M.BDN) AS BDN,
  { nnjoin(f'COUNT(DISTINCT CASE WHEN DN_NN{{i}}.VALUE <= {c.th} THEN M.BDN ELSE NULL END) AS NEG_BDN{{i}}')},
  { nnjoin(f'COUNT(DISTINCT CASE WHEN DN_NN{{i}}.VALUE > {c.th} THEN M.BDN ELSE NULL END) AS POS_BDN{{i}}')},
  { nnjoin(f'COUNT(DISTINCT CASE WHEN WL.RANK_BDN < {c.wl_th} AND DN_NN{{i}}.VALUE <= {c.th} THEN M.BDN ELSE NULL END) AS NEGWL_BDN{{i}}')},
  { nnjoin(f'COUNT(DISTINCT CASE WHEN WL.RANK_BDN < {c.wl_th} AND DN_NN{{i}}.VALUE > {c.th} THEN M.BDN ELSE NULL END) AS POSWL_BDN{{i}}')},
  SUM(M.QR) AS QR,
  SUM(M.Q) AS Q,
  SUM(M.R) AS R,
  SUM(M.NX) AS NX,
  { nnjoin(f'SUM(CASE WHEN DN_NN{{i}}.VALUE <= {c.th} THEN 1 ELSE 0 END) AS NEG_DN{{i}}') },
  { nnjoin(f'SUM(CASE WHEN DN_NN{{i}}.VALUE > {c.th} THEN 1 ELSE 0 END) AS POS_DN{{i}}') },
  { nnjoin(f'SUM(CASE WHEN WL.RANK_{c.wl_col} <= {c.wl_th} THEN 1 ELSE (CASE WHEN DN_NN{{i}}.VALUE <= {c.th} THEN 1 ELSE 0 END) END) AS NEGWL_DN{{i}}') },
  { nnjoin(f'SUM(CASE WHEN WL.RANK_{c.wl_col} <= {c.wl_th} THEN 0 ELSE (CASE WHEN DN_NN{{i}}.VALUE > {c.th} THEN 1 ELSE 0 END) END) AS POSWL_DN{{i}}') }
FROM
  MESSAGES M
  JOIN PCAP P ON M.PCAP_ID = P.ID
  JOIN PCAPMW ON M.PCAP_ID = PCAPMW.ID
  LEFT JOIN WHITELIST_DN AS WL ON M.DN_ID = WL.DN_ID
    { nnjoin('JOIN DN_NN{i} ON M.DN_ID = DN_NN{i}.DN_ID', sep="\n\t") }
GROUP BY
  M.PCAP_ID,
  P.Q,
  P.U,
  P.DURATION,
  PCAPMW.DGA,
  M.SLOTNUM
ORDER BY M.SLOTNUM
"""
    return sql



class DatasetFlags(enum.Flag):
    ALL = enum.auto()
    ONLY_1M = enum.auto()
    ONLY_TLD = enum.auto()


@dataclass
class Database:
    uri = f"postgresql+psycopg2://postgres:@localhost:5432/dns_mac"

    def __init__(self):
        self.engine = create_engine(self.uri)
        self.conn = self.engine.connect()
        pass


class Slot:
    def __init__(self, db: Database, config: SlotConfig):
        self.db = db
        self.config = config
        self.SLOTS_PER_DAY = (24 * 60 * 60) / config.sps
        self.HOUR_PER_SLOT = config.sps / (60 * 60)
        self.df = pd.read_sql(build_sql(config), self.db.engine).astype(int)
    pass

    def value_counts(self, columns: List):
        return self.df[columns + [ "slotnum"]].reset_index(drop=True).value_counts().unstack().T.fillna(0)

    def groupsum(self, column: str, use_timestamps=False, days=-1):
        df = self.df.copy()
        if days > 0:
            df = df[df["slotnum"] < self.SLOTS_PER_DAY * days]
            pass
        tmp = df[[column] + ["slotnum"]].groupby("slotnum").sum()
        tmp = tmp.unstack().T.fillna(0).reset_index(level=0, drop=True)
        if use_timestamps:
            last_row = tmp.iloc[-1]
            tmp_shifted = tmp.shift(fill_value=0)
            tmp = pd.concat([tmp_shifted, pd.Series([last_row], index=[tmp_shifted.shape[0]])])

        # for _ in range(len(groups)):
        #     tmp.reset_index(level=0, inplace=True)
        return tmp

    def slot(self, slotnum, mwtype: MWTYPE):
        sql = build_sql(self.config, slotnum, mwtype, with_dn=True)
        return pd.read_sql(sql, self.db.conn)





class Dataset:
    def __init__(self):
        
        df = pd.read_csv("./windows.csv", header=[0,1,2], index_col=0)

        cols = []
        for col in df.columns:
            col = list(col)
            if col[0] == "eps":
                col[2] = col[2][8:13] # [1:6]
            cols.append("_".join([a for a in col if a != "-"]))
            pass
        df.columns = cols

        df["duration"] = df["time_s_end"] - df["time_s_start"]
        df["time_s_start_norm"] = df.groupby("source_id")["time_s_start"].transform(lambda x: x - x.min())
        df["time_s_end_norm"] = df["time_s_start_norm"] + df["duration"]

        self.df = df

        self.DAY_SEC = 24 * 60 * 60
        pass
     
    def feature(self, flags: DatasetFlags = DatasetFlags.ALL):
        df = self.df[features_cols].copy()
        if DatasetFlags.ALL in flags:
            return df
        else:
            df = self.df
            if DatasetFlags.ONLY_1M in flags:
                tmp = []
                for col in df.columns:
                    if col[0] == 'llr' and col[2] == '1000000':
                        tmp.append(col)
                df = df[tmp]
                pass

            if DatasetFlags.ONLY_TLD in flags:
                tmp = []
                for col in df.columns:
                    if col[0] == 'llr' and col[1] == 'TLD':
                        tmp.append(col)
                df = df[tmp]
                pass

        return df.copy()
    
    def label(self):
        return self.df[label_col].copy()
    
    def slot(self, SPS = 1 * 60 * 60):
        self.SPS = SPS
        self.SLOTS_PER_DAY = self.DAY_SEC / SPS

        self.df["slot"] =  np.floor(self.df["time_s_end_norm"] / SPS)
        pass
    
    def slots(self, days=5):
        return self.SLOTS_PER_DAY * days

    
    def th(self, th, nn):
        cols = [ col for col in self.df.columns if col.startswith("eps_%s" % nn)]

        n_cols = [ col for col in cols if float(col.split("_")[2]) <= th]
        p_cols = [ col for col in cols if float(col.split("_")[2]) > th]

        self.df["neg"] = self.df[n_cols].sum(axis=1)
        self.df["pos"] = self.df[p_cols].sum(axis=1)
        pass




class XY:
    def __init__(self):
        self.X = np.ndarray(2)
        self.Y = np.ndarray(2)
    pass

class TT:
    def __init__(self):
        self.Train = XY()
        self.Test = XY()

    def split(self, dataset, label, test_size):
            tmp = train_test_split(dataset, label, test_size=test_size)

            self.Train.X = tmp[0]
            self.Test.X = tmp[1]
            self.Train.Y = tmp[2]
            self.Test.Y = tmp[3]

            for a in range(4):
                 print(tmp[a].shape)

            if self.Train.X.shape[0] != self.Train.Y.shape[0]:
                 raise Exception("Train sizes not match: %s != %s" % (self.Train.X.shape[0], self.Train.Y.shape[0]))
            
            if self.Test.X.shape[0] != self.Test.Y.shape[0]:
                 raise Exception("Test sizes not match: %s != %s" % (self.Test.X.shape[0], self.Test.Y.shape[0]))
            
            if self.Train.X.shape[0] == self.Test.X.shape[0]:
                 raise Exception("Train/Test X features not match: %s != %s" % (self.Train.X.shape, self.Test.X.shape))
            
            pass
    pass



def fpr(y, y_pred):
    false_positives = (y_pred[y == 0] > 0).sum()
    return false_positives / (y == 0).shape[0]

def fp(y, y_pred):
    return (y_pred[y == 0] > 0).sum()
    
def cm(y, y_pred):
    cm = confusion_matrix(y, y_pred)
    return { 'tn': cm[0, 0], 'fp': cm[0, 1], 'fn': cm[1, 0], 'tp': cm[1, 1] }

@dataclass
class Scoring:
    FPR = make_scorer(fpr, greater_is_better=True)
    ROC_AUC = "roc_auc"
    pass

class Trainer:
    def __init__(self, dataset: Dataset):
        self.dataset = dataset
        pass

    def save(self, model, filename):
        pickle.dump(model, open(filename, 'wb'))
        pass

    def load(self, filename):
        return pickle.load(open(filename, 'rb'))

    def ff(self, model, scorer, flags: DatasetFlags, cv=5, test_size=0.8):
        xy = TT()

        xy.split(self.dataset.feature(flags), self.dataset.label(), test_size=test_size)

        scorer = make_scorer(fpr, greater_is_better=True)
        sfs_fpr = SequentialFeatureSelector(
            model, 
            cv=cv,
            forward=False, 
            verbose=2,
            k_features=(1, xy.Train.X.shape[1]),
            scoring=scorer)

        with warnings.catch_warnings():
            warnings.filterwarnings("ignore")
            sfs_fpr = sfs_fpr.fit(xy.Train.X, xy.Train.Y)

        return sfs_fpr

    def stratifiedkfold(self, model, flags: DatasetFlags, n_splits=5, scoring = {"prec_macro": 'precision_macro', "fpr_macro": make_scorer(fpr)}):
        skf = StratifiedKFold(n_splits=n_splits, shuffle=True)

        scoring = {
            "prec_macro": 'precision_macro',
            "fpr_macro": Scoring.FPR,
            # "fp": make_scorer(fp),
            # "cm": make_scorer(cm)
        }

        return cross_validate(
            model,
            self.dataset.feature(flags),
            self.dataset.label(),
            cv=skf,
            scoring=scoring)
    
    