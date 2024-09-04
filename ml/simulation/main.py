import random
from typing import cast
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


def bo():
    df = DF
    
    FP = 250
    TP = 150
    
    days = 4
    

    bars = {
        'fp': { 'no': None, 'raised': None, 'missed': None, 'false': None },
        'tp': { 'no': None, 'raised': None, 'missed': None, 'false': None },
    }
    
    width = 0.6
    lw=1
    no_raised = False
    while True:
        hours = np.arange(24)
        fig, axs = plt.subplots(1, days, figsize=(20,4), sharey=True)
        axs = cast(np.ndarray, axs)
        while True:
            FPs_days = np.zeros((days, 24))
            TPs_days = np.zeros((days, 24))
            for day in range(days):
                FPs_days[day] = np.array([FP * traffic_model(hour, variance=0.15) for hour in hours])
                TPs_days[day] = (np.array([TP * traffic_model(hour, std_dev=18, variance=0.5) for hour in hours])) if day > 0 else np.zeros(24)
            
            FPs_days[:,0:4] += 15
            FPs_days[1:,0:4] *= 3
        
            for day in range(days):
                r = [0, 0, 8, 18][day]
                TPs_days[day][:r] = 0
                TPs_days[day][r+4:] = 0
        
            th = FPs_days[0].max()

            TOTs = FPs_days + TPs_days
            alarms  = TOTs > th
            true_no_alarms = (~alarms) & (TPs_days == 0) 
            true_raised_alarms = alarms & (TPs_days > 0)
            wrong_missed_alarms = ~alarms & (TPs_days > 0)
            wrong_false_alarms = alarms & (TPs_days == 0)

            one_day_undetected = not all([true_raised_alarms[day,:].any() for day in range(1, days)])
            only_fp = any([true_raised_alarms[day,:].sum() == 0 and wrong_false_alarms[day,:].sum() > 0 for day in range(1, days)])
            print(only_fp)
            atleast_three_falsepositive = True or wrong_false_alarms.sum() == 3
            if atleast_three_falsepositive and one_day_undetected and only_fp:
                break

        for day in range(days):
            FPs = FPs_days[day]
            TPs = TPs_days[day]

            if wrong_false_alarms[day].sum() > 0 and wrong_false_alarms[day].sum() > 0:
                legend_axes = day

            ax = axs[day]
            nocolor = (0,0,0,0)
            color = 'full'
            if color == 'blue':
                fpcolor = [ to_rgba("#a4c4ef") if not alarms[day][h] else to_rgba('#5C9DF7') for h in range(24) ]
                tpcolor = [ to_rgba("#3B5FFF", 0.65 if not alarms[day][h] else 1) for h in range(24) ]
                background_color = (0,0,0,0.05)
            elif color == 'black':
                fpcolor = [ to_rgba('black', 0.2) for h in range(24) ]
                tpcolor = [ to_rgba('black', 0.5) for h in range(24) ]
                background_color = 'white'
            elif color == 'full':
                falsecolor = np.zeros((24, 4), dtype=object)
                truecolor = np.zeros((24, 4), dtype=object)

                falsecolor[:,:] = (0,0,0,0.2)
                truecolor[:,:] = (0,0,0,0.8)

                falsecolor[true_raised_alarms[day].nonzero()] = to_rgba('green', 0.2)
                truecolor[true_raised_alarms[day].nonzero()] = to_rgba('green', 0.6)

                falsecolor[wrong_missed_alarms[day].nonzero()] = to_rgba('red', 0.2)
                truecolor[wrong_missed_alarms[day].nonzero()] = to_rgba('red', 0.6)

                falsecolor[wrong_false_alarms[day].nonzero()] = to_rgba('orange', 0.6)
                background_color = 'white'
                pass

            false_color = to_rgba("#4755ff", 1)
            missed_color = to_rgba("#E5446D")
        

            ax.axhline(th, linewidth=0.8, color='black', label="Threshold", linestyle="--" if day == 0 else "-")

            fp_bars = ax.bar(hours[FPs > 0], FPs[FPs > 0], bottom=0, width=width, color=falsecolor, label="FPs")
            if sum(TPs > 0):
                tp_bars = ax.bar(hours[TPs > 0], TPs[TPs > 0], bottom=FPs[TPs > 0], width=width, linewidth=0, edgecolor=nocolor, color=truecolor[TPs > 0], label="TPs")

            # if day > 0:
            #     ax.set_yticks([])
            ax.set_xticks([])
            ax.set_xlabel(f"${day+1}^{{{p.ordinal(day+1)[1:]}}}$ day hours")
            ax.set_ylim(0, TOTs.max() + 10)
            ax.set_xlim(-0.5,23.5)
            ax.set_facecolor(background_color)

            ax.legend()
            
            pass

            # handles, labels = axs[1].get_legend_handles_labels()

        
        fig.savefig("bo.pdf", bbox_inches="tight")

        plt.show()
        plt.close()
        pass

bo()