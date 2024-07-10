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

    # fig, axs = plt.subplots(1)
    fig, axs = plt.subplots(1, figsize=(4.5, 3))
    fig.tight_layout()

    queuestats, simstats, slotstats = cfg.parse_stats(args.traces[0])

    active = slotstats['arrivals']
    valid  = slotstats['occupied']

    active = active[valid != 0]
    valid  = valid[valid != 0]

    axs.plot(range(len(active/valid)), active/valid)

    axs.set_title('Saturation Fraction by Slot Index')
    axs.set_xlabel('Slot')
    axs.set_ylabel('Fraction of Cycles Saturated')

    axs.set_yscale('log')

    
    # locmaj = matplotlib.ticker.LogLocator(base=10,numticks=4) 
    # axs.yaxis.set_major_locator(locmaj)

    # locmin = matplotlib.ticker.LogLocator(base=10.0,subs=(0.2, 0.8),numticks=12)
    # locmin = matplotlib.ticker.LogLocator(base=10.0,subs=(0.2,0.4,0.6,0.8),numticks=12)
    # axs.yaxis.set_minor_locator(locmin)
    # axs.yaxis.set_minor_formatter(matplotlib.ticker.NullFormatter())

    # axs.set_yticks([.6])
    # axs.set_yticks([10**0, 8 * 10**-1, 6 * 10**-1, 4 * 10**-1, 2 * 10**-1, 10**-1])
    axs.set_ylim((10**0, 10**-1))

    print(active/valid)

    plt.savefig(args.outfile, bbox_inches="tight", dpi=500)
