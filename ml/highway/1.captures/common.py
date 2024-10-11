from dataclasses import dataclass
import enum
import itertools
import numpy as np
import pandas as pd
from sklearn.metrics import auc, roc_auc_score, roc_curve

def neg_rp(mu, sigma):
    try:
        _r = pow(mu,2)/(pow(sigma,2) - mu)
        _p = mu/pow(sigma,2)
        # print(f'{_r > 0 and (_p >= 0 and _p <=1)}\tmu={mu}\tsimga={sigma}\t_r={_r}\t_p={_p}')
        return _r, _p
    except:
        return neg_rp(mu - 1, sigma)
    

@dataclass()
class SigmaMu:
    mu: float
    sigma: float
    pass


class SimulateMode(enum.Enum):
    NORMAL = 0,
    NEGATIVE_BINOMIAL = 1
    pass


@dataclass()
class SimulateParams:
    fp: SigmaMu
    tp: SigmaMu
    mode: SimulateMode
    pass


def simulate(params: SimulateParams, n=30*24, p=10*24):
    if params.mode == SimulateMode.NORMAL:
        fps = np.random.normal(loc=params.fp.mu, scale=params.fp.sigma, size=n)
        tps = np.random.normal(loc=params.tp.mu, scale=params.tp.sigma, size=p)
    elif params.mode == SimulateMode.NEGATIVE_BINOMIAL:
        _r,_p = neg_rp(params.fp.mu, params.fp.sigma)
        fps = np.random.negative_binomial(_r, _p, size=n)
        _r,_p = neg_rp(params.tp.mu, params.tp.sigma)
        tps = np.random.negative_binomial(_r, _p, size=p)
        pass

    tps = np.concatenate([np.zeros(n-p), tps])
    labels = np.concatenate([np.zeros(n-p), np.ones(p)])

    fpr, tpr, thresholds = roc_curve(labels, (fps + tps), drop_intermediate=False)
    roc_auc = auc(fpr, tpr)
    optimal_idx = np.argmax((tpr - fpr)) # np.argmax((fpr < 0.05) * (tpr - fpr))

    mask = fpr < 0.01
    pAUCs_001 = roc_auc_score(labels, (fps + tps), max_fpr=0.01)
    if mask.sum() > 0:
        pAUC_001 = auc(fpr[mask], tpr[mask])
    else:
        pAUC_001 = 0.5
        pass
    optimal_idx_fpr001 = np.argmax((fpr < 0.01) * (tpr - fpr))

    th_001_range = (fpr < 0.01).sum() / len(fpr)

    return fpr, tpr, roc_auc, pAUCs_001, pAUC_001

def simulate_montecarlo(N, sp: SimulateParams, n=30*24, p=10*24):
    res = []
    for i in range(N):
        _ = simulate(sp, n, p)
        res.append(_)
        pass
    return pd.DataFrame(res, columns="fpr,tpr,roc_auc,pAUCs_001,pAUC_001".split(','))

def simulate_montecarlo_mean(N, sp: SimulateParams, n=30*24, p=10*24):
    df = simulate_montecarlo(N, sp, n, p)
    fpr,tpr,roc_auc,pAUCs_001,pAUC_001 = df.mean().to_list()
    return fpr,tpr,roc_auc,pAUCs_001,pAUC_001

def simulate_loop(N, mu_fp, mu_tp_ratio, sigma_fp_ratios, sigma_tp_ratios, sm: SimulateMode):
    dfs = []
    for sigma_fp_ratio, sigma_tp_ratio in list(itertools.product(sigma_fp_ratios, sigma_tp_ratios)):
        sigmamu_fp = SigmaMu(mu_fp, mu_fp * sigma_fp_ratio)
        sigmamu_tp = SigmaMu(sigmamu_fp.mu * mu_tp_ratio, sigmamu_fp.mu * mu_tp_ratio * sigma_tp_ratio)
        sp = SimulateParams(sigmamu_fp, sigmamu_tp, sm)
        df = simulate_montecarlo(N, sp)
        df['sigma_fp_ratio'] = sigma_fp_ratio
        df['sigma_tp_ratio'] = sigma_tp_ratio
        dfs.append(df)
        pass
    return pd.concat(dfs, axis=0)