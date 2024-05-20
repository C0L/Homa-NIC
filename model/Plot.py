import random
import math
import copy
import yaml
import argparse
import numpy as np
import matplotlib.pyplot as plt
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
        cfg = parsefn(trace)

        ts = []
        with open(trace) as file:
            for line in file:
                ts.append(float(line.rstrip()))
            
            axs.plot(ts)

            # count = 0
            # for i, line in enumerate(file):
            #     if i == 0:
            #         count = float(line.rstrip())
            #     if i == 1:
            #         qt = f"lw:{cfg['lw']}, hw:{cfg['hw']}, bs:{cfg['bs']}, cl:{cfg['cl']}, cs:{cfg['sl']}" 
            #         if qt not in cmap:
            #             cmap[qt] = c
            #             c = next(color)

            #         print(float(line.rstrip()))
            #         axs.plot(float(cfg['util']), float(line.rstrip()), 'o', c=cmap[qt], label=qt)

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


#     fig, axs = plt.subplots(1, figsize=(8,6))
#     # fig.tight_layout()
#     # TODO hackey 
#     # utils = [ .1, .2, .3, .4, .5, .6, .7, .8, .9, .99, .999, .9999, .99999 ]
# 
#     colors = ['r', 'g', 'b', 'm', 'c', 'y', 'k']
# 
#     i = 0
#     for trace in args.traces:
#         ys = []
#         # Get utilization
#         # print(util)
#         with open(trace) as file:
#             for line in file:
#                 ys.append(float(line.rstrip()))
# 
#         if 'srpt' in trace:
#             axs.plot(ys, colors[int(i/2)]+'--', label='srpt ' + trace.split('_')[1])
#         elif 'fifo' in trace:
#             axs.plot(ys, colors[int(i/2)]+':', label='fifo ' + trace.split('_')[1])
# 
#         i += 1
# 
#     axs.set_title(args.title)
#     axs.set_ylabel(args.ylabel)
#     axs.set_xlabel(args.xlabel)
#     # axs.set_ylim(0, 1000)
#     # axs.set_xlim(0,100)
#     # axs.set_title(r'Queue Flow Rate by Slot Index')
# 
#     axs.legend(loc='upper right', title='utilization')
#     # axs.legend(loc='upper center', bbox_to_anchor=(0.5, -0.1),
#     #       fancybox=True, shadow=True, ncol=1)
#     
#     # axs.set_ylabel('Flow Rate')
#     # axs.set_xlabel(r'Slot Index')
#     # axs.axhline(0, linestyle='--')
#     plt.savefig('relaxed_'+args.outfile, bbox_inches="tight")
# 
#     axs.set_xlim(0,100)
# 
#     plt.savefig('tight_'+args.outfile, bbox_inches="tight")
