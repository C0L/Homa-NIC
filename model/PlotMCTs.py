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

    fig, axs = plt.subplots(1, figsize=(8,6))

    # TODO need queue structure to turn into a tuple in label
    # Use a different color for each queue size
    # Perfect queue, 30/40, then 20/30, 10/20,
    # Need to check output stats to see if queue is unstable, use a red X if it is not?

    color = iter(cm.rainbow(np.linspace(0, 1, 8)))
    c = next(color)

    cmap = {}

    for trace in args.traces:
        cfg = parsefn(trace)
        print(cfg)
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

                    print(float(line.rstrip()))
                    axs.plot(float(cfg['util']), float(line.rstrip()), 'o', c=cmap[qt], label=qt)

                    

    axs.set_title("Mean Completition Time by Utilization")
    axs.set_ylabel("Mean Completion Time")
    axs.set_xlabel("Utilization")

    handles, labels = axs.get_legend_handles_labels()
    newLabels, newHandles = [], []
    for handle, label in zip(handles, labels):
        if label not in newLabels:
            newLabels.append(label)
            newHandles.append(handle)
    axs.legend(newHandles, newLabels)


    # axs.legend(loc='upper right', title='utilization')
    # axs.legend(loc='upper center', bbox_to_anchor=(0.5, -0.1),
    #            fancybox=True, shadow=True, ncol=4)

    # axs.set_xlim(0,1)
    
    plt.savefig(args.outfile, bbox_inches="tight")
