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

    fig, axs = plt.subplots(5, figsize=(5,6), sharex=True)
    fig.subplots_adjust(hspace=0)

    color = iter(cm.rainbow(np.linspace(0, 1, 12)))
    c = next(color)

    cmap = {}

    for trace in args.traces:
        cfg = parsefn(trace)

        wk = int(re.findall(r'\d+', cfg['workload'])[0])-1

        # print(cfg)
        with open(trace) as file:
            count = 0
            for i, line in enumerate(file):
                if i == 0:
                    count = float(line.rstrip())
                if i == 1:
                    qt = f"lw:{cfg['lw']}, hw:{cfg['hw']}, bs:{cfg['bs']}, cl:{cfg['cl']}, cs:{cfg['sl']}" 
                    if qt not in cmap:
                        cmap[qt] = c
                        c = next(color)

                    # print(float(line.rstrip()))
                    axs[wk].plot(float(cfg['util']), float(line.rstrip()), 'o', c=cmap[qt], label=qt)


    # axs.set_title("Mean Completition Time by Utilization")
    # axs.set_ylabel("Mean Completion Time")
    # axs.set_xlabel("Utilization")

    handles, labels = axs[0].get_legend_handles_labels()
    newLabels, newHandles = [], []
    for handle, label in zip(handles, labels):
        if label not in newLabels:
            newLabels.append(label)
            newHandles.append(handle)
    # axs[0].legend(newHandles, newLabels)


    # axs.legend(loc='upper right', title='utilization')
    axs[4].legend(newHandles, newLabels, loc='upper center', bbox_to_anchor=(0.5, -0.5),
                fancybox=False, shadow=False, ncol=2)


    # axs[4].legend(newHandles, newLabels, loc='upper center', bbox_to_anchor=(0.5, -0.1),
    #               fancybox=False, shadow=False, ncol=3)

    # axs.set_xlim(0,1)

    for i in range(5):
        # axs[i].set_xlim(0,200)
        # axs[i].set_ylim(0,.6)
        axs[i].text(.97, .9, 'w' + str(i+1), c='r', horizontalalignment='center', verticalalignment='center', transform = axs[i].transAxes)

    fig.text(0.5, 0.04, 'Tail Index', ha='center')
    fig.text(0, 0.5, 'Mean Completion Cycles', va='center', rotation='vertical')
    
    plt.savefig(args.outfile, bbox_inches="tight")
