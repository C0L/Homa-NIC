import cfg 
import random
import math
import copy
import yaml
import argparse
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.pyplot import cm
import matplotlib
import numpy as np
import scipy.stats
import sys
import re
from pathlib import Path

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
                    prog='Plot',
                    description='Plot traces')

    parser.add_argument('-t', '--traces', type=str, nargs='+', required=True)
    parser.add_argument('-f', '--outfile', type=str, required=True)

    args = parser.parse_args()

    fig, axs = plt.subplots(1, figsize=(2, 4.5))
    fig.tight_layout()

    queuestats, simstats, slotstats = cfg.parse_stats(args.traces[0])

    active = slotstats['arrivals']
    valid  = slotstats['occupied']

    active = active[valid != 0]
    valid  = valid[valid != 0]

    axs.plot(active/valid, range(len(active/valid)))

    axs.set_title('Underflow Fraction')
    axs.set_xlabel('Fraction')
    axs.set_ylabel('Slot')

    plt.savefig(args.outfile, bbox_inches="tight", dpi=500)
