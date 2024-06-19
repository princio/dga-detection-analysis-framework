import enum
from os import name
import pickle
from typing import Dict, List
import warnings
import pandas as pd
from dataclasses import dataclass
from mlxtend.feature_selection import SequentialFeatureSelector
import psycopg2
from sklearn.metrics import confusion_matrix, make_scorer
from sklearn.model_selection import RandomizedSearchCV, train_test_split, cross_validate, StratifiedKFold, cross_val_predict
import numpy as np
from sqlalchemy import create_engine

csv = None

label_col = ('-',      'source',             'mc')

features_cols = [
    # (  '-',           '-',      'n_message'),
    
    # (  '-',           '-',     'fn_req_min'),
    # (  '-',           '-',     'fn_req_max'),
    
    # (  '-',           '-',   'time_s_start'),
    # (  '-',           '-',     'time_s_end'),
    (  '-',           '-',       'duration'),
    
    (  '-',           '-',             'qr'),
    (  '-',           '-',              'u'),
    (  '-',           '-',              'q'),
    (  '-',           '-',              'r'),
    (  '-',           '-',             'nx'),
    
    (  '-', 'whitelisted',              '0'),
    (  '-', 'whitelisted',             '100'),
    (  '-', 'whitelisted',             '1000'),
    (  '-', 'whitelisted',             '1000000'),
    
    ('eps',        'NONE', '[0.000, 0.100)'),
    ('eps',        'NONE', '[0.100, 0.250)'),
    ('eps',        'NONE', '[0.250, 0.500)'),
    ('eps',        'NONE', '[0.500, 0.900)'),
    ('eps',        'NONE', '[0.900, 1.100)'),
    
    ('eps',         'TLD', '[0.000, 0.100)'),
    ('eps',         'TLD', '[0.100, 0.250)'),
    ('eps',         'TLD', '[0.250, 0.500)'),
    ('eps',         'TLD', '[0.500, 0.900)'),
    ('eps',         'TLD', '[0.900, 1.100)'),
    
    ('eps',       'ICANN', '[0.000, 0.100)'),
    ('eps',       'ICANN', '[0.100, 0.250)'),
    ('eps',       'ICANN', '[0.250, 0.500)'),
    ('eps',       'ICANN', '[0.500, 0.900)'),
    ('eps',       'ICANN', '[0.900, 1.100)'),
    
    ('eps',     'PRIVATE', '[0.000, 0.100)'),
    ('eps',     'PRIVATE', '[0.100, 0.250)'),
    ('eps',     'PRIVATE', '[0.250, 0.500)'),
    ('eps',     'PRIVATE', '[0.500, 0.900)'),
    ('eps',     'PRIVATE', '[0.900, 1.100)'),
    
    ('llr',        'ninf',           'NONE'),
    ('llr',        'ninf',            'TLD'),
    ('llr',        'ninf',          'ICANN'),
    ('llr',        'ninf',        'PRIVATE'),
    
    ('llr',        'pinf',           'NONE'),
    ('llr',        'pinf',            'TLD'),
    ('llr',        'pinf',          'ICANN'),
    ('llr',        'pinf',        'PRIVATE'),
    
    ('llr',        'NONE',              '0'),
    ('llr',        'NONE',             '100'),
    ('llr',        'NONE',             '1000'),
    ('llr',        'NONE',             '1000000'),
    
    ('llr',         'TLD',              '0'),
    ('llr',         'TLD',             '100'),
    ('llr',         'TLD',             '1000'),
    ('llr',         'TLD',             '1000000'),
    
    ('llr',       'ICANN',              '0'),
    ('llr',       'ICANN',             '100'),
    ('llr',       'ICANN',             '1000'),
    ('llr',       'ICANN',             '1000000'),
    
    ('llr',     'PRIVATE',              '0'),
    ('llr',     'PRIVATE',             '100'),
    ('llr',     'PRIVATE',             '1000'),
    ('llr',     'PRIVATE',             '1000000')
]


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
    def __init__(self, db: Database, SEC_PER_SLOT, TH, DATASET="CTU-13", onlyfirsts=False):
        self.db = db
        self.SEC_PER_SLOT = SEC_PER_SLOT
        self.SLOTS_PER_DAY = (24 * 60 * 60) / SEC_PER_SLOT
        self.HOUR_PER_SLOT = SEC_PER_SLOT / (60 * 60)
        self.TH = TH
        self.DATASET = DATASET
        all_or_firsts = "(SELECT DISTINCT ON (DN_ID, PCAP_ID) * FROM MESSAGE)" if onlyfirsts else "MESSAGE"
        self.df = pd.read_sql(f"""
WITH WHITELIST AS (SELECT * FROM WHITELIST_DN WHERE WHITELIST_ID = 1),
DN_NN1 AS (SELECT * FROM DN_NN WHERE NN_ID = 1),
DN_NN2 AS (SELECT * FROM DN_NN WHERE NN_ID = 2),
DN_NN3 AS (SELECT * FROM DN_NN WHERE NN_ID = 3),
DN_NN4 AS (SELECT * FROM DN_NN WHERE NN_ID = 4),
PCAPMW AS (SELECT PCAP.ID, DGA FROM PCAP JOIN MALWARE ON PCAP.MALWARE_ID = MALWARE.ID WHERE PCAP.DATASET='{DATASET}'),
MESSAGES AS (
  SELECT 
    __M.DN_ID,
    __M.PCAP_ID, 
    FLOOR(__M.TIME_S_TRANSLATED / ({SEC_PER_SLOT})) AS SLOTNUM, 
    COUNT(*) AS QR,
    SUM(CASE WHEN __M.IS_R IS FALSE THEN 1 ELSE 0 END) AS Q,
    SUM(CASE WHEN __M.IS_R IS TRUE THEN 1 ELSE 0 END) AS R,
    SUM(CASE WHEN __M.IS_R IS TRUE AND RCODE = 3 THEN 1 ELSE 0 END ) AS NX
  FROM
      {all_or_firsts} __M
  GROUP BY
    __M.DN_ID,
    __M.PCAP_ID,
    SLOTNUM
)
SELECT
  M.PCAP_ID,
  P.Q AS QQ,
  P.U AS UU,
  P.DURATION AS D,
  PCAPMW.DGA,
  M.SLOTNUM,
  SUM(CASE WHEN WHITELIST.RANK_BDN > 1000000 THEN 1 ELSE 0 END) AS RANK_BDN,
  COUNT(DISTINCT M.DN_ID) AS U,
  SUM(M.QR) AS QR,
  SUM(M.Q) AS Q,
  SUM(M.R) AS R,
  SUM(M.NX) AS NX,
  SUM(CASE WHEN DN_NN1.VALUE <= {TH} THEN 1 ELSE 0 END) AS NEG_NN1,
  SUM(CASE WHEN DN_NN2.VALUE <= {TH} THEN 1 ELSE 0 END) AS NEG_NN2,
  SUM(CASE WHEN DN_NN3.VALUE <= {TH} THEN 1 ELSE 0 END) AS NEG_NN3,
  SUM(CASE WHEN DN_NN4.VALUE <= {TH} THEN 1 ELSE 0 END) AS NEG_NN4,
  SUM(CASE WHEN DN_NN1.VALUE > {TH} THEN 1 ELSE 0 END) AS POS_NN1,
  SUM(CASE WHEN DN_NN2.VALUE > {TH} THEN 1 ELSE 0 END) AS POS_NN2,
  SUM(CASE WHEN DN_NN3.VALUE > {TH} THEN 1 ELSE 0 END) AS POS_NN3,
  SUM(CASE WHEN DN_NN4.VALUE > {TH} THEN 1 ELSE 0 END) AS POS_NN4,
  SUM(CASE WHEN WHITELIST.RANK_BDN <= 1000000 THEN 1 ELSE (CASE WHEN DN_NN1.VALUE <= 0.5 THEN 1 ELSE 0 END) END) AS NEGWL_NN1,
  SUM(CASE WHEN WHITELIST.RANK_BDN <= 1000000 THEN 1 ELSE (CASE WHEN DN_NN2.VALUE <= 0.5 THEN 1 ELSE 0 END) END) AS NEGWL_NN2,
  SUM(CASE WHEN WHITELIST.RANK_BDN <= 1000000 THEN 1 ELSE (CASE WHEN DN_NN3.VALUE <= 0.5 THEN 1 ELSE 0 END) END) AS NEGWL_NN3,
  SUM(CASE WHEN WHITELIST.RANK_BDN <= 1000000 THEN 1 ELSE (CASE WHEN DN_NN4.VALUE <= 0.5 THEN 1 ELSE 0 END) END) AS NEGWL_NN4,
  SUM(CASE WHEN WHITELIST.RANK_BDN <= 1000000 THEN 0 ELSE (CASE WHEN DN_NN1.VALUE > 0.5 THEN 1 ELSE 0 END) END) AS POSWL_NN1,
  SUM(CASE WHEN WHITELIST.RANK_BDN <= 1000000 THEN 0 ELSE (CASE WHEN DN_NN2.VALUE > 0.5 THEN 1 ELSE 0 END) END) AS POSWL_NN2,
  SUM(CASE WHEN WHITELIST.RANK_BDN <= 1000000 THEN 0 ELSE (CASE WHEN DN_NN3.VALUE > 0.5 THEN 1 ELSE 0 END) END) AS POSWL_NN3,
  SUM(CASE WHEN WHITELIST.RANK_BDN <= 1000000 THEN 0 ELSE (CASE WHEN DN_NN4.VALUE > 0.5 THEN 1 ELSE 0 END) END) AS POSWL_NN4
FROM
  MESSAGES M
  JOIN PCAP P ON M.PCAP_ID = P.ID
  JOIN PCAPMW ON M.PCAP_ID = PCAPMW.ID
  LEFT JOIN WHITELIST ON M.DN_ID = WHITELIST.DN_ID
  JOIN DN_NN1 ON M.DN_ID = DN_NN1.DN_ID
  JOIN DN_NN2 ON M.DN_ID = DN_NN2.DN_ID
  JOIN DN_NN3 ON M.DN_ID = DN_NN3.DN_ID
  JOIN DN_NN4 ON M.DN_ID = DN_NN4.DN_ID
GROUP BY
  M.PCAP_ID,
  P.Q,
  P.U,
  P.DURATION,
  PCAPMW.DGA,
  M.SLOTNUM
ORDER BY M.SLOTNUM
""", self.db.engine).astype(int)
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
    
    def slot(self, SEC_PER_SLOT = 1 * 60 * 60):
        self.SEC_PER_SLOT = SEC_PER_SLOT
        self.SLOTS_PER_DAY = self.DAY_SEC / SEC_PER_SLOT

        self.df["slot"] =  np.floor(self.df["time_s_end_norm"] / SEC_PER_SLOT)
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
    
    