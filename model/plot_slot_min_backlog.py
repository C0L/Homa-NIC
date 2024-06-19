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
                      ('cycles', np.uint64),
                      ('sum_comps', np.uint64),
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

        axs[wk].plot(minbacklog[minbacklog != 0], label=cfg['util'])

    for i in range(5):
        axs[i].text(.85, .85, 'w' + str(i+1), c='r', horizontalalignment='center', verticalalignment='center', transform = axs[i].transAxes)
        axs[i].set_xlabel("Slot Index")
        axs[i].set_ylabel("Min. Slot Backlog (Cycles)")
        axs[i].ticklabel_format(axis='y', style='sci', scilimits=(0, 0))
        axs[i].set_ylim(ymin=0)

    handles, labels = axs[0].get_legend_handles_labels()
    axs[4].legend(handles, labels, loc='upper center', bbox_to_anchor=(0.5, -0.5),
                     fancybox=False, shadow=False, ncol=2, title="Utilization")


    # TODO 1MB limit

    # for trace in args.traces:
    #     cfg = parsefn(trace)
    #     print(cfg)

    #     wk = int(re.findall(r'\d+', cfg['workload'])[0])-1

    #     simstats  = np.fromfile(trace, dtype=simstat_t, count=1)
    #     slotstats = np.fromfile(trace, dtype=slotstat_t, count=-1, offset=simstats.nbytes)

    #     mintotalbacklog = slotstats['mintotalbacklog'].astype(np.uint64)
    #     minbacklog = slotstats['minbacklog'].astype(np.uint64)
    #     np.set_printoptions(threshold=sys.maxsize)

    #     validcycles = slotstats['validcycles']
    #     backlog = slotstats['backlog']
    #     totalbacklog = slotstats['totalbacklog']

    #     axs[wk, 0].plot((totalbacklog[validcycles != 0]/validcycles[validcycles != 0]), label=cfg['util'])
    #     axs[wk, 1].plot((backlog[validcycles != 0]/validcycles[validcycles != 0]), label=cfg['util'])

    #     axs[wk, 2].plot(mintotalbacklog[mintotalbacklog != 18446744073709551615], label=cfg['util'])
    #     axs[wk, 3].plot(minbacklog[minbacklog != 18446744073709551615], label=cfg['util'])

    #     validcycles = slotstats['validcycles']

    #     axs[wk, 4].plot((validcycles[validcycles != 0]/simstats['cycles']), label=cfg['util'])

    # for i in range(5):
    #     axs[i,0].ticklabel_format(axis='y', style='sci', scilimits=(0, 0))
    #     axs[i,1].ticklabel_format(axis='y', style='sci', scilimits=(0, 0))

    #     axs[i,2].ticklabel_format(axis='y', style='sci', scilimits=(0, 0))
    #     axs[i,3].ticklabel_format(axis='y', style='sci', scilimits=(0, 0))

    #     # axs[i,4].set_yscale('log')
    #     axs[i,4].ticklabel_format(axis='y', style='sci', scilimits=(0, 0))

    #     axs[i,0].text(.05, .85, 'w' + str(i+1), c='r', horizontalalignment='center', verticalalignment='center', transform = axs[i,0].transAxes)
    #     axs[i,0].set_xlabel("Slot Index")
    #     axs[i,0].set_ylabel("Avg. Total Backlog")

    #     axs[i,1].text(.05, .85, 'w' + str(i+1), c='r', horizontalalignment='center', verticalalignment='center', transform = axs[i,1].transAxes)
    #     axs[i,1].set_xlabel("Slot Index")
    #     axs[i,1].set_ylabel("Avg. Slot Backlog")

    #     axs[i,2].text(.05, .85, 'w' + str(i+1), c='r', horizontalalignment='center', verticalalignment='center', transform = axs[i,2].transAxes)
    #     axs[i,2].set_xlabel("Slot Index")
    #     axs[i,2].set_ylabel("Min. Total Backlog")

    #     axs[i,3].text(.05, .85, 'w' + str(i+1), c='r', horizontalalignment='center', verticalalignment='center', transform = axs[i,3].transAxes)
    #     axs[i,3].set_xlabel("Slot Index")
    #     axs[i,3].set_ylabel("Min. Slot Backlog")

    #     axs[i,4].text(.05, .85, 'w' + str(i+1), c='r', horizontalalignment='center', verticalalignment='center', transform = axs[i,4].transAxes)
    #     axs[i,4].set_xlabel("Slot Index")
    #     axs[i,4].set_ylabel("Ratio Occupied")

    #     axs[i,0].set_ylim(ymin=0)
    #     axs[i,1].set_ylim(ymin=0)

    #     axs[i,2].set_ylim(ymin=0)
    #     axs[i,3].set_ylim(ymin=0)

    #     # axs[i,4].set_ylim(ymin=.001)

    #     axs[i,1].axhline(y=1000000/64, color='r', linestyle='-', label='1MB')
    #     axs[i,3].axhline(y=1000000/64, color='r', linestyle='-', label='1MB')

    # handles, labels = axs[0, 0].get_legend_handles_labels()

    # newLabels, newHandles = [], []
    # for handle, label in zip(handles, labels):
    #     if label not in newLabels:
    #         newLabels.append(label)
    #         newHandles.append(handle)

    # axs[4,0].legend(handles, labels, loc='upper center', bbox_to_anchor=(0.5, -0.5),
    #                 fancybox=False, shadow=False, ncol=2)

    # handles, labels = axs[0, 1].get_legend_handles_labels()
    # axs[4,1].legend(handles, labels, loc='upper center', bbox_to_anchor=(0.5, -0.5),
    #                 fancybox=False, shadow=False, ncol=2)

    # handles, labels = axs[0, 2].get_legend_handles_labels()

    # newLabels, newHandles = [], []
    # for handle, label in zip(handles, labels):
    #     if label not in newLabels:
    #         newLabels.append(label)
    #         newHandles.append(handle)

    # axs[4,2].legend(handles, labels, loc='upper center', bbox_to_anchor=(0.5, -0.5),
    #                 fancybox=False, shadow=False, ncol=2)

    # handles, labels = axs[0, 3].get_legend_handles_labels()
    # axs[4,3].legend(handles, labels, loc='upper center', bbox_to_anchor=(0.5, -0.5),
    #                 fancybox=False, shadow=False, ncol=2)

    # handles, labels = axs[0, 4].get_legend_handles_labels()
    # axs[4,4].legend(handles, labels, loc='upper center', bbox_to_anchor=(0.5, -0.5),
    #                 fancybox=False, shadow=False, ncol=2)

    # axs[0,0].set_title('Total Backlog')
    # axs[0,1].set_title('Slot Backlog')
    # axs[0,2].set_title('Min Total Backlog')
    # axs[0,3].set_title('Min Slot Backlog')
    # axs[0,4].set_title('Ratio Slot Occupied')

    axs[0].set_title('Minimum Slot Backlog')
    
    plt.savefig(args.outfile, bbox_inches="tight")
