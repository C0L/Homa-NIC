import random
import math
import copy
import yaml
import argparse
import numpy as np
import matplotlib
import matplotlib.pyplot as plt
from matplotlib.pyplot import cm 
import numpy as np
import scipy.stats
import sys
import re
from pathlib import Path

def parsefn(fn):
    fsplit = Path(fn).stem.split('_')
    return {
        'workload' : fsplit[0], 
        'util'     : fsplit[1], 
    }


slotstat_t = np.dtype([('validcycles', np.uint64),
                       ('totalbacklog', np.uint64),
                       ('backlog', np.uint64),
                       ('mintotalbacklog', np.uint64),
                       ('minbacklog', np.uint64)])

simstat_t = np.dtype([('highwater', np.uint64),
                      ('lowwater', np.uint64),
                      ('max', np.uint64),
                      ('packets', np.uint64),
                      ('comps', np.uint64),
                      ('sum_comps', np.uint64),
                      ('cycles', np.uint64),
                      ('inacc', np.uint64),
                      ('sum_slow_down', np.uint64)])

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
                    prog='Plot',
                    description='Plot traces')

    parser.add_argument('-t', '--traces', type=str, nargs='+', required=True)
    parser.add_argument('-f', '--outfile', type=str, required=True)

    args = parser.parse_args()

    fig, axs = plt.subplots(5, figsize=(4,10), sharex=False)

    fig.tight_layout()

    color = iter(cm.rainbow(np.linspace(0, 1, 12)))
    c = next(color)

    cmap = {}

    for trace in args.traces:
        cfg = parsefn(trace)
        print(cfg)

        wk = int(re.findall(r'\d+', cfg['workload'])[0])-1

        # if (wk >= 3):
        #     continue

        simstats  = np.fromfile(trace, dtype=simstat_t, count=1)
        slotstats = np.fromfile(trace, dtype=slotstat_t, count=-1, offset=simstats.nbytes)

        mintotalbacklog = slotstats['mintotalbacklog'].astype(np.uint64)
        minbacklog = slotstats['minbacklog'].astype(np.uint64)
        np.set_printoptions(threshold=sys.maxsize)

        validcycles = slotstats['validcycles']
        backlog = slotstats['backlog']
        totalbacklog = slotstats['totalbacklog']

        validcycles = slotstats['validcycles']

        axs[wk].plot((validcycles[validcycles != 0]/simstats['cycles']), label=cfg['util'])


    for i in range(5):
        axs[i].text(.85, .85, 'w' + str(i+1), c='r', horizontalalignment='center', verticalalignment='center', transform = axs[i].transAxes)
        axs[i].set_xlabel("Slot Index")
        axs[i].set_ylabel("Ratio Occupied")
        # axs[i].ticklabel_format(axis='y', style='sci', scilimits=(0, 0))


        axs[i].set_yscale('log')
        # axs[i].set_ylim(ymax=10**0)
        axs[i].set_yticks([10**0, 10**-1, 10**-2, 10**-3, 10**-4, 10**-5, 10**-6, 10**-7])
        # axs[i].set_yticks([10**0, 10**-1, 10**-2, 10**-3, 10**-4, 10**-5, 10**-6, 10**-7, 10**-8, 10**-9, 10**-10])
        # axs[i].set_ylim(10**-7,10**.5)
        # axs[i].y_loglog()
        # axs[i].set_ylim(ymin=.0001)

    # plt.yscale('log')

    handles, labels = axs[0].get_legend_handles_labels()
    axs[4].legend(handles, labels, loc='upper center', bbox_to_anchor=(0.5, -0.5),
                     fancybox=False, shadow=False, ncol=2, title="Utilization")

    axs[0].set_title('Ratio Slot Occupied')
   
    plt.savefig(args.outfile, bbox_inches="tight")
