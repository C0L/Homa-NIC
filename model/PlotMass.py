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
        'arrival'  : fn[7],
        'type'     : fn[8]
    }

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
                    prog='Plot',
                    description='Plot traces')

    parser.add_argument('-t', '--traces', type=str, nargs='+', required=True)
    parser.add_argument('-f', '--outfile', type=str, required=True)

    args = parser.parse_args()

    fig, axs = plt.subplots(1, figsize=(8,6))

    color = iter(cm.rainbow(np.linspace(0, 1, 8)))
    c = next(color)

    cmap = {}

    for trace in args.traces:
        print(trace)
        cfg = parsefn(trace)

        ts = []
        with open(trace) as file:
            for line in file:
                ts.append(float(line.rstrip()))
            
            axs.plot(ts, label=cfg['type'] + ' ' + cfg['util'])

    axs.set_title("Mass by Slot")
    axs.set_ylabel("Mass")
    axs.set_xlabel("Slot")

    handles, labels = axs.get_legend_handles_labels()
    newLabels, newHandles = [], []
    for handle, label in zip(handles, labels):
        if label not in newLabels:
            newLabels.append(label)
            newHandles.append(handle)

    axs.legend(newHandles, newLabels)

    axs.set_xlim(0,400)
    axs.set_ylim(0,1500)
    plt.savefig(args.outfile, bbox_inches="tight")
