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

    fig, axs = plt.subplots(5,4, figsize=(14,10), sharex=False)

    fig.tight_layout()

    slotstat_t = np.dtype([('validcycles', np.uint64), ('totalbacklog', np.uint64), ('backlog', np.uint64), ('mintotalbacklog', np.uint64), ('minbacklog', np.uint64)])

    color = iter(cm.rainbow(np.linspace(0, 1, 12)))
    c = next(color)

    cmap = {}

    # TODO 1MB limit

    for trace in args.traces:
        cfg = parsefn(trace)
        print(cfg)

        wk = int(re.findall(r'\d+', cfg['workload'])[0])-1
        qt = f"lw:{cfg['lw']}, hw:{cfg['hw']}, bs:{cfg['bs']}, cl:{cfg['cl']}, cs:{cfg['sl']}"

        stats = np.fromfile(trace, dtype=slotstat_t)

        print(stats['mintotalbacklog'])

        mintotalbacklog = stats['mintotalbacklog'].astype(np.uint64)
        minbacklog = stats['minbacklog'].astype(np.uint64)

        axs[wk, 0].plot(stats['totalbacklog']/stats['validcycles'], 'o', label=cfg['util'])
        axs[wk, 1].plot(stats['backlog']/stats['validcycles'], 'o', label=cfg['util'])
        
        axs[wk, 2].plot(mintotalbacklog[mintotalbacklog != 18446744073709551615], 'o', label=cfg['util'])
        axs[wk, 3].plot(minbacklog[minbacklog != 18446744073709551615], 'o', label=cfg['util'])

    for i in range(5):
        axs[i,0].ticklabel_format(axis='y', style='sci', scilimits=(0, 0))
        axs[i,1].ticklabel_format(axis='y', style='sci', scilimits=(0, 0))

        axs[i,2].ticklabel_format(axis='y', style='sci', scilimits=(0, 0))
        axs[i,3].ticklabel_format(axis='y', style='sci', scilimits=(0, 0))

        axs[i,0].text(.05, .85, 'w' + str(i+1), c='r', horizontalalignment='center', verticalalignment='center', transform = axs[i,0].transAxes)
        axs[i,0].set_xlabel("Slot Index")
        axs[i,0].set_ylabel("Avg. Total Backlog")

        axs[i,1].text(.05, .85, 'w' + str(i+1), c='r', horizontalalignment='center', verticalalignment='center', transform = axs[i,1].transAxes)
        axs[i,1].set_xlabel("Slot Index")
        axs[i,1].set_ylabel("Avg. Slot Backlog")

        axs[i,2].text(.05, .85, 'w' + str(i+1), c='r', horizontalalignment='center', verticalalignment='center', transform = axs[i,0].transAxes)
        axs[i,2].set_xlabel("Slot Index")
        axs[i,2].set_ylabel("Min. Total Backlog")

        axs[i,3].text(.05, .85, 'w' + str(i+1), c='r', horizontalalignment='center', verticalalignment='center', transform = axs[i,1].transAxes)
        axs[i,3].set_xlabel("Slot Index")
        axs[i,3].set_ylabel("Min. Slot Backlog")

        axs[i,2].set_ylim(ymin=0)
        axs[i,3].set_ylim(ymin=0)

        axs[i,3].axhline(y=1000000/64, color='r', linestyle='-', label='1MB')


    handles, labels = axs[0, 0].get_legend_handles_labels()

    newLabels, newHandles = [], []
    for handle, label in zip(handles, labels):
        if label not in newLabels:
            newLabels.append(label)
            newHandles.append(handle)

    axs[4,0].legend(handles, labels, loc='upper center', bbox_to_anchor=(0.5, -0.5),
                    fancybox=False, shadow=False, ncol=2, title='Utilization')

    handles, labels = axs[0, 1].get_legend_handles_labels()
    axs[4,1].legend(handles, labels, loc='upper center', bbox_to_anchor=(0.5, -0.5),
                    fancybox=False, shadow=False, ncol=2, title='Utilization')

    # ym = [.00005, .00004, .00002, .00002, .00005]


    #     # axs[i].text(.97, .9, 'w' + str(i+1), c='r', horizontalalignment='center', verticalalignment='center', transform = axs[i].transAxes)
    #     # axs[i,0].set_ylim(ymin=0, ymax=ym[i])
   
    fig.text(.1, 1, 'Total Backlog by Slot Index', va='center')
    fig.text(.25, 1, 'Slot Backlog by Slot Index', va='center')

    fig.text(.5, 1, 'Min Total Backlog by Slot Index', va='center')
    fig.text(.7, 1, 'Min Slot Backlog by Slot Index', va='center')
    
    plt.savefig(args.outfile, bbox_inches="tight")
