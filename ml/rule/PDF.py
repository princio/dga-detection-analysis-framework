from dataclasses import dataclass
import enum
from typing import Optional
from matplotlib import pyplot as plt
import numpy as np
from scipy.stats import nbinom
from scipy.stats import norm
from sklearn.metrics import auc, roc_curve


FPR = 0.00182
TPR = 0.999916

class PDFRNG(enum.Enum):
    POISSON = 0
    PASCAL = 1
    pass

@dataclass()
class PDF:
    n: int

    mu_n: float
    mu_p: float

    _sigma_n: Optional[float]
    sigma_p: float

    FPR: float
    TPR: float

    discost: float = 0.5
    discost_prob: float = 0.4
    rng = np.random.default_rng()

    @property
    def mu_fp(self):
        return self.mu_n * FPR

    @property
    def mu_tp(self):
        return self.mu_p * TPR

    @property
    def sigma_n(self) -> float:
        if self._sigma_n is None:
            raise Exception('Sigma N not calculated or unset.')
        return self._sigma_n

    @property
    def sigma_fp(self) -> float:
        return self.sigma_n * FPR

    @property
    def sigma_tp(self):
        return self.sigma_p * TPR

    def _sigma(self, mu, discost, prob_discost):
        Z_high = norm.ppf(0.5 + prob_discost / 2)
        sigma = (discost * mu) / Z_high
        return sigma
    
    def set_sigma_n(self, discost, prob_discost=0.4):
        self.discost = discost
        self.prob_discost = prob_discost
        Z_high = norm.ppf(0.5 + prob_discost / 2)
        self._sigma_n = (discost * self.mu_n) / Z_high
        return self
    
    def _negbin_cdf(self, mu, sigma, thresholds):
        print(mu, sigma)
        sigma_squared = (sigma)**2
        r = mu**2 / (sigma_squared - mu)
        p = r / (r + mu)
        return [1 - nbinom.cdf(t, r, p) for t in thresholds]
    
    def fp_negbin_cdf(self, thresholds):
        return self._negbin_cdf(self.mu_fp, self.sigma_fp, thresholds)
    
    def tp_negbin_cdf(self, thresholds):
        return self._negbin_cdf(self.mu_tp, self.sigma_tp, thresholds)
    
    def _negbin_gen(self, mu, sigma, n_samples):
        sigma_squared = (sigma)**2
        r = mu**2 / (sigma_squared - mu)
        p = r / (r + mu)
        # return np.random.negative_binomial(r, p, size=n_samples)
        return self.rng.negative_binomial(self.mu_n, 1 - self.FPR, size=n_samples)
        # return self.rng.poisson(self.mu_n * self.FPR, size=n_samples)
    
    def _poisson_gen(self, lam, n_samples):
        return self.rng.poisson(lam, size=n_samples)
    
    def _pascal_gen(self, r, p, n_samples):
        return self.rng.negative_binomial(r, p, size=n_samples)

    def fp_gen(self, rng: PDFRNG, n_samples: int):
        if rng == PDFRNG.POISSON:
            return self._poisson_gen(self.mu_n * self.FPR, n_samples)
        elif rng == PDFRNG.PASCAL:
            return self._pascal_gen(self.mu_n, 1 - self.FPR, n_samples)
        else:
            raise Exception(f"Distribution {rng} not supported.")
    
    def tp_gen(self, rng: PDFRNG, n_samples: int):
        if rng == PDFRNG.POISSON:
            return self._poisson_gen(self.mu_p * self.TPR, n_samples)
        elif rng == PDFRNG.PASCAL:
            return self._pascal_gen(self.mu_p, self.TPR, n_samples)
        else:
            raise Exception(f"Distribution {rng} not supported.")
    
    def roc(self, axs, balance, rng: PDFRNG):
        n = 30 * 24
        p = int(n * balance)

        fps = self.fp_gen(rng, n)
        tps = np.concatenate([np.zeros(n-p), self.tp_gen(PDFRNG.POISSON, p)])

        labels = np.concatenate([np.zeros(n-p), np.ones(p)])

        fpr, tpr, thresholds = roc_curve(labels, (fps + tps))
        roc_auc = auc(fpr, tpr)
        optimal_idx = np.argmax(tpr - fpr)

        return thresholds[optimal_idx], fpr[optimal_idx], tpr[optimal_idx], roc_auc

    def __str__(self):
        return (
            f"PDF(mu_n={self.mu_n}, mu_p={self.mu_p}, "
            f"_sigma_n={self._sigma_n}, sigma_p={self.sigma_p}, "
            f"_sigma_fp={self.sigma_fp}, sigma_tp={self.sigma_tp}, "
            f"FPR={self.FPR}, TPR={self.TPR}, "
            f"discost={self.discost}, discost_prob={self.discost_prob})"
        )
    pass
