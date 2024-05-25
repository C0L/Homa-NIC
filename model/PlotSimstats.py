import random
import math
import copy
import yaml
import argparse
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.pyplot import cm 
import numpy as np
import scipy.stats
import sys
import re

def parsefn(fn):
    fn = fn.split('_')
    return {
        'workload' : fn[0], 
        'util'     : fn[1], 
        'hw'       : fn[2],
        'lw'       : fn[3],
        'bs'       : fn[4],
        'cl'       : fn[5],
        'sl'       : fn[6],
        'arrival'  : fn[7]
    }

# TODO create some matplotlib defaults here

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
                    prog='Plot',
                    description='Plot traces')

    parser.add_argument('-t', '--traces', type=str, nargs='+', required=True)
    parser.add_argument('-f', '--outfile', type=str, required=True)

    args = parser.parse_args()

    # fig, axs = plt.subplots(5, figsize=(5,6), sharex=True)
    # fig, axs = plt.subplots(5,2, figsize=(8,10), sharex=False)

    fig, axs = plt.subplots(5,2, figsize=(8,10), sharex=False)

    # fig.subplots_adjust(hspace=0)
    fig.tight_layout()

    color = iter(cm.rainbow(np.linspace(0, 1, 12)))
    c = next(color)

    cmap = {}

    for trace in args.traces:
        cfg = parsefn(trace)
        print(cfg)

        wk = int(re.findall(r'\d+', cfg['workload'])[0])-1
        qt = f"lw:{cfg['lw']}, hw:{cfg['hw']}, bs:{cfg['bs']}, cl:{cfg['cl']}, cs:{cfg['sl']}"

        stats = np.fromfile(trace, dtype=np.uint64)
        print(stats)

        if qt not in cmap:
            cmap[qt] = c
            c = next(color)

            axs[wk, 0].plot(float(cfg['util']), stats[0]/stats[6], '^', c=cmap[qt], label=f"({cfg['lw']}, {cfg['hw']})")
            axs[wk, 0].plot(float(cfg['util']), stats[1]/stats[6], 'v', c=cmap[qt], label=f"({cfg['lw']}, {cfg['hw']})")

            axs[wk, 1].plot(float(cfg['util']), stats[2]/stats[3], 'o', c=cmap[qt], label=f"({cfg['lw']}, {cfg['hw']})")
        else:
            axs[wk, 0].plot(float(cfg['util']), stats[0]/stats[6], '^', c=cmap[qt])
            axs[wk, 0].plot(float(cfg['util']), stats[1]/stats[6], 'v', c=cmap[qt])

            axs[wk, 1].plot(float(cfg['util']), stats[2], 'o', c=cmap[qt])

    handles, labels = axs[0, 0].get_legend_handles_labels()

    # newLabels, newHandles = [], []
    # for handle, label in zip(handles, labels):
    #     if label not in newLabels:
    #         newLabels.append(label)
    #         newHandles.append(handle)

    axs[4,0].legend(handles, labels, loc='upper center', bbox_to_anchor=(0.5, -0.5),
                    fancybox=False, shadow=False, ncol=3, title='High(^) and Low(v) Water Marks')

    handles, labels = axs[0, 1].get_legend_handles_labels()
    axs[4,1].legend(handles, labels, loc='upper center', bbox_to_anchor=(0.5, -0.5),
                    fancybox=False, shadow=False, ncol=3, title='High Water Mark')

    # ym = [.00005, .00004, .00002, .00002, .00005]
    for i in range(5):
        axs[i,0].text(.05, .85, 'w' + str(i+1), c='r', horizontalalignment='center', verticalalignment='center', transform = axs[i,0].transAxes)
        axs[i,0].set_xlabel("Utilization")
        axs[i,0].set_ylabel("Event Ratio")

        axs[i,1].text(.05, .85, 'w' + str(i+1), c='r', horizontalalignment='center', verticalalignment='center', transform = axs[i,1].transAxes)
        axs[i,1].set_xlabel("Utilization")
        axs[i,1].set_ylabel("Occupancy")

        # axs[i].text(.97, .9, 'w' + str(i+1), c='r', horizontalalignment='center', verticalalignment='center', transform = axs[i].transAxes)
        # axs[i,0].set_ylim(ymin=0, ymax=ym[i])
        axs[i,0].set_ylim(ymin = 0)
        axs[i,1].set_ylim(ymin = 0)

        # axs[i,0].set_ylim(ymin=0, ymax=.00001)
        # axs[i,1].set_ylim(ymin=0, ymax=.00001)

    # fig.text(0.5, 0.04, 'Utilization', ha='center')
    # fig.text(-.05, 0.5, 'Count', va='center', rotation='vertical')


    fig.text(.13, 1, 'Highwater and Lowater Mark Ratio of Events', va='center')
    fig.text(.6, 1, 'Maximum Queue Occupancy Reached', va='center')
    
    plt.savefig(args.outfile, bbox_inches="tight")
