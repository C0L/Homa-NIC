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
from statsmodels.stats.weightstats import DescrStatsW

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
                    prog='Plot',
                    description='Plot traces')

    parser.add_argument('-t', '--traces', type=str, nargs='+', required=True)
    parser.add_argument('-f', '--outfile', type=str, required=True)

    args = parser.parse_args()

    fig, axs = plt.subplots(1, figsize=(4.5,2))

    fig.tight_layout()

    underflows = []
    for trace in args.traces:
        queuestats, simstats, slotstats = cfg.parse_stats(trace)

        underflow = simstats['underflow']
        underflows.append(underflow[0])

    print(underflows)
    # axs.set_xlabel("")
    # axs.set_ylabel("")

    # plt.savefig(args.outfile, bbox_inches="tight", dpi=500)
