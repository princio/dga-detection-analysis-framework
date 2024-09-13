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
import random

matplotlib.use('Qt5Agg')

DF = pd.read_csv("simulated_dns_traffic.csv").pivot(index=["day","hour"], columns="variance", values="domains").reset_index()

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
    healthies = np.zeros(24)
    malicious = np.zeros(24)
    _fp = np.zeros(24)
    _tp = np.zeros(24)
    _tn = np.zeros(24)
    _fn = np.zeros(24)

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
        return self
    
    def copy(self):
        o = Traffic()
        o.healthies = np.copy(self.healthies)
        o.malicious = np.copy(self.malicious)
        o.fill()
        return o
    pass


def bo():
    df = DF
    
    magn_healthy = 50000
    magn_malicious_healthies = 10
    def healthy_traffic(magnitude): #[ 55000, 55000, 48000, 50000]
        traffic = Traffic()
        traffic.healthies = np.array([magnitude * traffic_model(hour, std_dev=8, variance=0.15) for hour in range(24)])
        return traffic
    
    def malicious_traffic(p_magnitude, n_magnitude=1): # [0, 0, 30, 50]
        traffic = Traffic()
        traffic.healthies = np.array([magn_malicious_healthies * traffic_model(hour, std_dev=18, variance=0.5) for hour in range(24) ])
        traffic.malicious = np.array([p_magnitude * traffic_model(hour, std_dev=18, variance=0.5) for hour in range(24) ])
        return traffic

    def day1():
        ht = healthy_traffic(55000)
        mt = Traffic()
        return ht.fill(), mt.fill()

    def day2(th):
        ok = False
        for i in range(10000):
            ht = healthy_traffic(55000)
            mt = Traffic()
            if ((ht.fp() - th) > 15).sum() > 1:
                ok = True
                break
            pass
        
        if not ok:
            print("Day2 not found.")
            exit(1)

        return ht.fill(), mt.fill()

    def day3(th):
        ok = False
        for i in range(100):
            if ok:
                break
            ht = healthy_traffic(48000)
            
            for j in range(1000):
                mt = malicious_traffic(30)

                for _ in range(12):
                    mt.malicious[random.randint(0,23)] = 0
                for _ in range(8):
                    mt.malicious[random.randint(0,23)] *= 0.05

                if (((mt.tp() + ht.fp()) - th) >= -5).sum() == 0:
                    ok = True
                    break
                pass
            pass

        if not ok:
            print("Day3 not found.")
            exit(1)

        return ht.fill(), mt.fill()

    def day4(mt: Traffic, th):
        ok = False
        for i in range(1000):
            ht = healthy_traffic(50000)

            if (((mt.tp() + ht.fp()) - th) >= 5).sum() > 1:
                ok = True
                break
            pass
    
        if not ok:
            print("Day4 not found.")
            exit(1)
    
        return ht.fill(), mt

    days = 4
    
    width = 0.6
    ok = False
    for kkk in range(100):
        hours = np.arange(24)
        fig, axs = plt.subplots(3, days, figsize=(20,4), sharey="row")
        axs = cast(np.ndarray, axs)
        if False:
            for jjj in range(1000):
                ht = healthy_traffic()
                mt = malicious_traffic()

                ht.malicious[:,0] = 0
                mt.healthies[:,0] = 0
                mt.malicious[:,0] = 0

                for day in range(1,4):
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
                pass
        
        ht1,mt2 = day1()
        th = ht1.fp().max()
        ht2,mt2 = day2(th)
        ht3,mt3 = day3(th)
        ht4,mt4 = day4(mt3, th)

        traffic_days = [  
            (ht1,mt2),
            (ht2,mt2),
            (ht3,mt3),
            (ht4,mt4)
        ]

        # if not ok:
        #     exit(1)
    
        print('ok')

        ymax = math.ceil(max([ ht.fp().max() + mt.tp().max() for ht, mt in traffic_days]) / 100) * 100

        for day in range(days):

            # if wrong_false_alarms[day].sum() > 0 and wrong_false_alarms[day].sum() > 0:
            #     legend_axes = day

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


            fp = traffic_days[day][0].fp()
            tp = traffic_days[day][1].tp()
            tp[tp<5] = 0
            
            axs[0, day].bar(hours, fp, bottom=0, width=width, color=healthycolor + positivemask, label="HT-FP")
            axs[0, day].set_ylim(0, ymax)


            axs[1, day].bar(hours, tp, bottom=0,  width=width, color=healthycolor + positivemask)
            axs[1, day].set_ylim(0, ymax)


            axs[2, day].bar(hours, fp, bottom=0,       width=width, color=healthycolor + positivemask, label="HT-FP + MT-FP)")
            axs[2, day].bar(hours, tp, bottom=fp,     width=width, color=maliciouscolor + positivemask, label="MT-TP")
            if tp.sum() > 0:
                alarms = ((tp > 0) & ((fp + tp) > th))
                noalarms = ((tp > 0) & ((fp + tp) <= th))
                axs[2, day].bar(hours[alarms], (fp + tp)[alarms], lw=1, width=width, edgecolor='royalblue', color=healthycolor)
                axs[2, day].bar(hours[noalarms], (fp + tp)[noalarms], lw=1, width=width, edgecolor='red', color=healthycolor)

            alarms = ((fp) > th) & (tp == 0)
            axs[2, day].bar(hours[alarms], (fp + tp)[alarms], lw=1, width=width, edgecolor='orange', color=healthycolor)
            axs[2, day].set_ylim(0, ymax)



            # if day > 0:
            #     ax.set_yticks([])
            for i in range(3):
                axs[i, day].set_xticks([])
                axs[i, day].set_facecolor(background_color)
                # axs[i, day].legend()

            hours_labels_pos = [0, 7, 15, 23]
            hours_labels = [
                '$1^{{st}}$ hr',
                '$8^{th}$',
                '$16^{th}$',
                '$24^{th}$'
            ]
            axs[-1, day].set_xticks(hours_labels_pos, hours_labels)
            # axs[-1, day].set_xlabel('hours')


            pad = 16
            axs[0, 0].set_ylabel("False Positives (FPs)", rotation='vertical', labelpad=pad, size='medium')
            axs[1, 0].set_ylabel("True Positives (TPs)", rotation='vertical', labelpad=pad, size='medium')
            axs[2, 0].set_ylabel("TPs + FPs", rotation='vertical', labelpad=pad, size='medium')
            
            if False:
                axs[-1, day].set_xlabel(f"${day+1}^{{{p.ordinal(day+1)[1:]}}}$ day hours\n" + [
                    "setting the threshold",    
                    "false positive alarms",
                    "missed detection",  
                    "infection detected",  
                ][day], size='medium')
            else:
                axs[0, day].set_title([
                    "\"setting the threshold\" day",
                    "\"false positive alarms\" day",
                    "\"missed detection\" day",  
                    "\"infection detected\" day",  
                ][day], size='medium')
        
            axs[-1, day].axhline(th, linewidth=0.8, color='black', label="Threshold", linestyle="--" if day == 0 else "-")
            
            pass

        for i in range(axs.shape[0]):
            for ax in axs[i,:]:
                ax.set_ylim(axs[0,0].get_ylim())

            # handles, labels = axs[1].get_legend_handles_labels()

        fig.set_size_inches(14,8)
        fig.subplots_adjust(left=0.08, top=0.95, right=0.98, bottom=0.05,
        hspace=0.1, wspace=0.05)
        # fig.tight_layout()
        fig.savefig("bo.pdf")#, bbox_inches="tight")

        plt.show()
        plt.close()
        pass

bo()