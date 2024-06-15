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
        'priority' : fsplit[2]
    }

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

    fig, axs = plt.subplots(3, figsize=(4,10), sharex=False)

    fig.tight_layout()

    color = iter(cm.rainbow(np.linspace(0, 1, 12)))
    c = next(color)

    cmap = {}

    xs_slowdown = []
    ys_slowdown = []

    xs_fct = []
    ys_fct = []

    xs_inacc = []
    ys_inacc = []

    for trace in args.traces:
        cfg = parsefn(trace)
        # print(cfg)

        simstats = np.fromfile(trace, dtype=simstat_t, count=1)
        xs_slowdown.append(int(cfg['priority']))
        ys_slowdown.append(simstats['sum_slow_down'][0]/simstats['comps'][0])

        xs_fct.append(int(cfg['priority']))
        ys_fct.append(simstats['sum_comps'][0]/simstats['comps'][0])

        xs_inacc.append(int(cfg['priority']))
        ys_inacc.append(simstats['inacc'][0]/simstats['packets'][0])

    wk = int(re.findall(r'\d+', cfg['workload'])[0])

    axs[0].text(.85, .85, 'w' + str(wk), c='r', horizontalalignment='center', verticalalignment='center', transform = axs[0].transAxes)
    axs[1].text(.85, .85, 'w' + str(wk), c='r', horizontalalignment='center', verticalalignment='center', transform = axs[1].transAxes)
    axs[2].text(.85, .85, 'w' + str(wk), c='r', horizontalalignment='center', verticalalignment='center', transform = axs[2].transAxes)

    sort = sorted(zip(xs_slowdown, ys_slowdown))
    print(sort)

    xs_slowdown, ys_slowdown= zip(*sort)

    axs[0].set_yscale('log')
    # print(sorted(slowdowns))
    axs[0].plot(xs_slowdown[:], ys_slowdown, 'o', label=cfg['util'])
    #axs[0].set_xlim((0, 50))
    # axs[0].set_xlim((0, 400))
    # axs[0].set_ylim((0, 2000))
    axs[0].set_xlabel("Priorities")
    axs[0].set_ylabel("Slowdown")
    #axs[0].loglog()

    axs[1].plot(xs_fct, ys_fct, 'o', label=cfg['util'])
    #axs[1].set_xlim((0, 50))
    # axs[1].set_ylim((0, 2000))
    axs[1].set_xlabel("Priorities")
    axs[1].set_ylabel("FCT")
    axs[1].set_yscale('log')

    sort = sorted(zip(xs_inacc, ys_inacc))
    print(sort)

    # sort = sorted(zip(xs_inacc, ys_inacc))
    # print(sort)

    xs_inacc, ys_inacc = zip(*sort)

    axs[2].plot(xs_inacc, ys_inacc, 'o', label=cfg['util'])
    # axs[2].set_xlim((0, 400))
    #axs[2].set_xlim((0, 50))
    #axs[2].set_ylim((0, 100))
    axs[2].set_xlabel("Priorities")
    axs[2].set_ylabel("Inaccuracy Rate")
    axs[2].set_yscale('log')

    # handles, labels = axs[0].get_legend_handles_labels()
    # axs[4].legend(handles, labels, loc='upper center', bbox_to_anchor=(0.5, -0.5),
    #                  fancybox=False, shadow=False, ncol=2, title="Utilization")

    # axs[0].set_title('Ratio Slot Occupied')
   
    plt.savefig(args.outfile, bbox_inches="tight")
