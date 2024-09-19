from dataclasses import dataclass
import random
import numpy as np


class NotOkException(Exception):
    def __init__(self, message) -> None:
        super().__init__(message)
        pass
    pass


def traffic_model(hour, variance=0.2, std_dev=8):
    peak_hour = 13
    
    # Gaussian function centered at peak_hour
    base_traffic = np.exp(-0.8 * ((hour - peak_hour) / std_dev) ** 2)
    
    # Adding variability
    traffic = base_traffic + np.random.normal(1, variance)
    return max(0, traffic)  # Ensure non-negative values

class SlotGroup24:
    def __init__(self, FPR = 0.00216, TPR = 0.99744) -> None:
        self.FPR: float = FPR
        self.TPR: float = TPR
        self._healthy = np.zeros((2, 24), dtype=np.int32)
        self._malicious = np.zeros((2, 24), dtype=np.int32)
        pass

    def healthy(self, magnitude):
        self._healthy[0, :] = np.array([magnitude * traffic_model(hour, std_dev=8, variance=0.15) for hour in range(24)], dtype=np.int32)
        return self
    
    def malicious(self, p_magnitude, n_magnitude=1):
        self._malicious[0, :] = np.array([n_magnitude * traffic_model(hour, std_dev=8, variance=0.15) for hour in range(24)], dtype=np.int32)
        self._malicious[1, :] = np.array([p_magnitude * traffic_model(hour, std_dev=8, variance=0.15) for hour in range(24)], dtype=np.int32)
        return self
    
    def both(self, h_magnitude, p_m_magnitude, n_m_magnitude):
        self.healthy(h_magnitude)
        self.malicious(p_m_magnitude, n_m_magnitude)
        return self
    
    def h(self):
        return self._healthy[0, :] + self._malicious[0, :]
    
    def mp(self):
        return self._malicious[1, :]

    def fp(self):
        return np.ceil(self.h() * self.FPR)
    
    def tp(self):
        return np.ceil(self.mp() * self.TPR)
    
    def tn(self):
        return np.ceil(self.h() * (1 - self.FPR))
    
    def fn(self):
        return np.ceil(self.mp() * (1 - self.TPR))
    
    def n(self):
        return self.fn() + self.tn()
    
    def p(self):
        return self.fp() + self.tp()
    
    def fpa(self, th):
        return (self.p() > th) & (self.mp() == 0)
    
    def tpa(self, th):
        return (self.p() > th) & (self.mp() > 0)
    
    def tna(self, th):
        return (self.p() <= th) & (self.mp() == 0)
    
    def fna(self, th):
        return (self.p() <= th) & (self.mp() > 0)
    
    def pa(self):
        return (self.mp() == 0)
    
    def na(self):
        return (self.mp() > 0)
    
    def copy(self):
        o = SlotGroup24(self.FPR, self.TPR)
        o._healthy = np.copy(self._healthy)
        o._malicious = np.copy(self._malicious)
        return o
    pass
