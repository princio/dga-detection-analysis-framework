from dataclasses import dataclass
import math
import random
from typing import List, cast
from matplotlib import patches
from matplotlib.collections import PatchCollection
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import inflect
import matplotlib
from matplotlib.colors import to_rgba

matplotlib.use('Qt5Agg')

DF = pd.read_csv("simulated_dns_traffic.csv").pivot(index=["day","hour"], columns="variance", values="domains").reset_index()
DF

# Genera un numero casuale tra 0 e 24
random_number = random.randint(0, 24)
print(random_number)


def traffic_model(hour, variance=0.2, std_dev=8):
    peak_hour = 13
    
    # Gaussian function centered at peak_hour
    base_traffic = np.exp(-0.8 * ((hour - peak_hour) / std_dev) ** 2)
    
    # Adding variability
    traffic = base_traffic + np.random.normal(0, variance)
    return max(0, traffic)  # Ensure non-negative values



p = inflect.engine()



FPR = 0.00216
TPR = 0.99744
@dataclass()
class Traffic:
    healthies = np.zeros((24, 4))
    malicious = np.zeros((24, 4))
    _fp = np.zeros((24, 4))
    _tp = np.zeros((24, 4))
    _tn = np.zeros((24, 4))
    _fn = np.zeros((24, 4))

    def fp(self):
        if (self._fp == 0).all():
            self._fp = self.healthies * FPR
        return self._fp
    
    def tp(self):
        if (self._tp == 0).all():
            self._tp = self.malicious * TPR
        return self._tp
    
    def tn(self):
        if (self._tn == 0).all():
            self._tn = self.healthies * (1 - FPR)
        return self._tn
    
    def fn(self):
        if (self._fn == 0).all():
            self._fn = self.malicious * (1 - TPR)
        return self._fn
    
    def fill(self):
        self.fp()
        self.tp()
        self.fn()
        self.tp()
        pass
    pass

def bo():
    df = DF
    
    magn_healthy = 50000
    magn_malicious_healthies = 10
    def healthy_traffic():
        traffic = Traffic()
        traffic.healthies = np.array([np.array([ [ 55000, 55000, 48000, 50000][day] * traffic_model(hour, std_dev=8, variance=0.15) for hour in range(24) ]) for day in range(4) ]).T
        return traffic
    
    def malicious_traffic():
        traffic = Traffic()
        traffic.healthies = np.array([np.array([ magn_malicious_healthies * traffic_model(hour, std_dev=18, variance=0.5) for hour in range(24) ]) for day in range(4) ]).T
        traffic.malicious = np.array([np.array([ [0, 0, 30, 50][day] * traffic_model(hour, std_dev=18, variance=0.5) for hour in range(24) ]) for day in range(4) ]).T
        return traffic

    days = 4
    
    width = 0.6
    ok = False
    for kkk in range(100):
        hours = np.arange(24)
        fig, axs = plt.subplots(3, days, figsize=(20,4), sharey="row")
        axs = cast(np.ndarray, axs)
        for jjj in range(1000):
            ht = healthy_traffic()
            mt = malicious_traffic()

            ht.malicious[:,0] = 0
            mt.healthies[:,0] = 0
            mt.malicious[:,0] = 0

            for day in range(1,4):
                r = [0, 0, 8, 18][day]
                # mt.malicious[:r, day] *= 0.2 # type: ignore
                # mt.malicious[r+6:, day] *= 0.2 # type: ignore
                import random
                for _ in range(12):
                    mt.malicious[random.randint(0,23), day] = 0
                for _ in range(8):
                    mt.malicious[random.randint(0,23), day] *= 0.05
            
            mt.malicious[mt.malicious < 10] = 0

            ht.fill()
            mt.fill()
            
            th = ht._fp[:, 0].max()

            TOTs = ht._fp + mt._tp + mt._fp

            alarms  = TOTs > th
            true_no_alarms = (~alarms) & (mt.malicious == 0) 
            true_raised_alarms = alarms & (mt.malicious > 0)
            wrong_missed_alarms = ~alarms & (mt.malicious > 0)
            wrong_false_alarms = alarms & (mt.malicious == 0)

            one_day_undetected = any([(true_raised_alarms[:, day].sum() == 0) and (wrong_missed_alarms[:, day].sum() > 0) and (wrong_false_alarms[:, day].sum() == 0) for day in range(1, days)])
            one_day_only_false_alarms = any([(true_raised_alarms[:, day].sum() == 0) & (wrong_false_alarms[:, day].sum() > 0) for day in range(1, days)])
            atleast_three_falsepositive = wrong_false_alarms.sum() == 3
            if atleast_three_falsepositive and one_day_undetected and one_day_only_false_alarms:
                ok = True
                break
        if not ok:
            exit(1)
        for day in range(days):

            if wrong_false_alarms[day].sum() > 0 and wrong_false_alarms[day].sum() > 0:
                legend_axes = day

            healthycolor = np.full((24, 4), np.array(to_rgba('grey', 0), dtype=np.float64), dtype=np.float64)
            maliciouscolor = np.full((24, 4), np.array(to_rgba('black', 0), dtype=np.float64), dtype=np.float64)
            
            negativemask = np.full((24, 4), np.array([0,0,0,0.2]), dtype=np.float64)
            positivemask = np.full((24, 4), np.array([0,0,0,0.6]), dtype=np.float64)

            background_color = to_rgba('gray', 0.1)

            fff = 0
            # axs[fff, day].bar(hours, ht.healthies[:, day], bottom=0,                      width=width, color=healthycolor + negativemask, label="HT")
            # fff += 1

            # axs[fff, day].bar(hours, mt.healthies[:, day], bottom=0,                      width=width, color=healthycolor + negativemask, label="HT")
            # axs[fff, day].bar(hours, mt.malicious[:, day], bottom=mt.healthies[:, day],   width=width, color=maliciouscolor + negativemask, label="HT")
            # fff += 1
            
            axs[fff, day].bar(hours, ht._fp[:, day], bottom=0, width=width, color=healthycolor + positivemask, label="HT-FP")

            axs[fff, day].set_ylim(0, math.ceil(ht._fp[:, :].max() / 100) * 100)
            fff += 1

            axs[fff, day].bar(hours, mt._tp[:, day], bottom=0,   width=width, color=maliciouscolor + positivemask)
            axs[fff, day].bar(hours, mt._fp[:, day], bottom=mt._tp[:, day],   width=width, color=healthycolor + positivemask)
            axs[fff, day].set_ylim(0, math.ceil((mt._tp[:, day] + mt._fp[:, day]).max() / 100) * 100)
            fff += 1

            axs[fff, day].bar(hours, ht._fp[:, day] + mt._fp[:, day], bottom=0,       width=width, color=healthycolor + positivemask, label="HT-FP + MT-FP)")
            axs[fff, day].bar(hours, mt._tp[:, day], bottom=ht._fp[:, day] + mt._fp[:, day],     width=width, color=maliciouscolor + positivemask, label="MT-TP")
            if mt.malicious[:, day].sum() > 0:
                alarms = ((mt._tp[:, day] > 0) & ((ht._fp[:, day] + mt._tp[:, day]) > th))
                noalarms = ((mt._tp[:, day] > 0) & ((ht._fp[:, day] + mt._tp[:, day]) <= th))
                axs[fff, day].bar(hours[alarms], (ht._fp[:, day] + mt._tp[:, day])[alarms], lw=1, width=width, edgecolor='royalblue', color=healthycolor)
                axs[fff, day].bar(hours[noalarms], (ht._fp[:, day] + mt._tp[:, day])[noalarms], lw=1, width=width, edgecolor='red', color=healthycolor)

            alarms = ((ht._fp[:, day]) > th) & (mt._tp[:, day] == 0)
            axs[fff, day].bar(hours[alarms], (ht._fp[:, day] + mt._tp[:, day])[alarms], lw=1, width=width, edgecolor='orange', color=healthycolor)
            axs[fff, day].set_ylim(0, math.ceil((ht._fp[:, day] + mt._fp[:, day] + mt._tp[:, day]).max() / 100) * 100)
            fff += 1

            # if day > 0:
            #     ax.set_yticks([])
            for i in range(3):
                axs[i, day].set_xticks([])
                axs[i, day].set_facecolor(background_color)
                # axs[i, day].legend()

            axs[-1, day].set_xticks([0, 7, 15, 23], [1, 8, 16, 24])

            pad = 35
            axs[0, 0].set_ylabel("healthy\nfalse positives", rotation='horizontal', labelpad=pad, size='small')
            axs[1, 0].set_ylabel("malicious\npositives", rotation='horizontal', labelpad=pad, size='small')
            axs[2, 0].set_ylabel("all positives", rotation='horizontal', labelpad=pad, size='small')
            
            axs[-1, day].set_xlabel(f"${day+1}^{{{p.ordinal(day+1)[1:]}}}$ day hours\n" + [
                "setting the threshold",
                "false positive alarms",
                "missed detection",  
                "infection detected",  
            ][day], size='small')
        
            axs[-1, day].axhline(th, linewidth=0.8, color='black', label="Threshold", linestyle="--" if day == 0 else "-")
            
            pass


            # handles, labels = axs[1].get_legend_handles_labels()

        
        fig.savefig("bo.pdf", bbox_inches="tight")

        plt.show()
        plt.close()
        pass

bo()