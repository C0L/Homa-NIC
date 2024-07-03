import cfg 
import random
import math
import copy
import yaml
import argparse
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.pyplot import cm
import matplotlib
import numpy as np
import scipy.stats
import sys
import re
from pathlib import Path
from statsmodels.stats.weightstats import DescrStatsW

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
                    prog='Plot',
                    description='Plot traces')

    parser.add_argument('-t', '--traces', type=str, nargs='+', required=True)
    parser.add_argument('-f', '--outfile', type=str, required=True)

    args = parser.parse_args()

    fig, axs = plt.subplots(1, figsize=(4.5,2))
    ax2 = axs.twiny()

    fig.tight_layout()

    color = iter(cm.rainbow(np.linspace(0, 1, 12)))
    c = next(color)

    ptr = {
        'w1' : 10,
        'w2' : 11,
        'w3' : 6
    }

    lw = 3
    for trace in args.traces:
        queuestats, simstats, slotstats = cfg.parse_stats(trace)

        totalbacklog = slotstats['arrivalbacklog']
        validcycles  = slotstats['arrivals']

        occupied = slotstats['occupied']
        occupied = occupied[occupied != 0]

        tts = ((totalbacklog[validcycles != 0]/validcycles[validcycles != 0])[0::]*5/1000)
        print(occupied.shape)

        percentiles = [.5, .1, .01]
        pp = []

        for perc in percentiles:
            print(perc)

            wq = DescrStatsW(data=occupied, weights=occupied)
            p = wq.quantile(probs=perc, return_pandas=False)
            print("p")
            print(p)
            p = (abs(occupied-p).argmin())
            print(p)
            pp.append(p)
            print(sum(occupied[p::])/sum(occupied))
            axs.axvline(x=p, linestyle='--', c='r')

        axs.set_xlim((0, len(tts)))
        ax2.set_xlim(axs.get_xlim())
        ax2.set_xticks(pp)
        ax2.set_xticklabels(['p50', 'p90', 'p99'])
        ax2.set_xlabel('Slot Occupancy')

        axs.plot(tts, linewidth=2)
        sub = cfg.add_subplot_axes(axs, [0.1,0.6,0.3,0.3])
        sub.plot(tts[0:33], linewidth=2)
        sub.plot(16, tts[16], 'g*')
        sub.text(18, tts[16], '({}, {})'.format(16, round(tts[16])), c='r', fontsize='x-small')
        # sub.annotate('',xy=(ptr[cfg['workload']],tts[ptr[cfg['workload']]]),xytext=(5,tts[30]), c='r', arrowprops={'width': 2, 'headwidth': 8, 'color':'red'})

        lw -= 1


    # axs.text(.95, .05, cfg['workload'], c='r', horizontalalignment='center', verticalalignment='center', transform = axs.transAxes)
    axs.set_xlabel("Slot Index")
    axs.set_ylabel(r"Average TTS $(\mu s)$")

    plt.savefig(args.outfile, bbox_inches="tight", dpi=500)
