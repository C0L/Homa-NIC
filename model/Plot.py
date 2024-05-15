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

def parse(path):
    data = []
    file  = open(path, 'r')
    lines = file.readlines()
         
    for line in lines:
        data.append(float(line.split()[0]))

    return data

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
                    prog='Plot',
                    description='Plot traces')

    parser.add_argument('-t', '--traces', type=str, nargs='+', required=True)
    parser.add_argument('-f', '--outfile', type=str, required=True)
    parser.add_argument('-y', '--ylabel', type=str, required=True)
    parser.add_argument('-x', '--xlabel', type=str, required=True)
    parser.add_argument('-T', '--title', type=str, required=True)

    args = parser.parse_args()

    fig, axs = plt.subplots(1, figsize=(8,6))
    # fig.tight_layout()
    # TODO hackey 
    # utils = [ .1, .2, .3, .4, .5, .6, .7, .8, .9, .99, .999, .9999, .99999 ]

    colors = ['r', 'g', 'b', 'm', 'c', 'y', 'k']

    i = 0
    for trace in args.traces:
        ys = []
        # Get utilization
        # print(util)
        with open(trace) as file:
            for line in file:
                ys.append(float(line.rstrip()))

        if 'srpt' in trace:
            axs.plot(ys, colors[int(i/2)]+'--', label='srpt ' + trace.split('_')[1])
        elif 'fifo' in trace:
            axs.plot(ys, colors[int(i/2)]+':', label='fifo ' + trace.split('_')[1])

        i += 1

    axs.set_title(args.title)
    axs.set_ylabel(args.ylabel)
    axs.set_xlabel(args.xlabel)
    # axs.set_ylim(0, 1000)
    # axs.set_xlim(0,100)
    # axs.set_title(r'Queue Flow Rate by Slot Index')

    axs.legend(loc='upper right', title='utilization')
    # axs.legend(loc='upper center', bbox_to_anchor=(0.5, -0.1),
    #       fancybox=True, shadow=True, ncol=1)
    
    # axs.set_ylabel('Flow Rate')
    # axs.set_xlabel(r'Slot Index')
    # axs.axhline(0, linestyle='--')
    plt.savefig('relaxed_'+args.outfile, bbox_inches="tight")

    axs.set_xlim(0,100)

    plt.savefig('tight_'+args.outfile, bbox_inches="tight")
