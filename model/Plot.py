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

    args = parser.parse_args()

    fig, axs = plt.subplots(1, figsize=(8,6))
    fig.tight_layout(pad=3.0)

    i = 0
    for trace in args.traces:
        axs.plot(parse(trace), label=trace.split('_')[1])
        # axs.plot(t, label=[args.traces.split(',')[i]])
        i += 1

    # axs.set_ylim(0, .2)
    axs.set_title(r'Queue Flow Rate by Slot Index')
    axs.legend(loc='upper left', title='Rate')
    axs.set_ylabel('Flow Rate')
    axs.set_xlabel(r'Slot Index')
    axs.axhline(0, linestyle='--')
    plt.savefig(args.outfile)
