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
from scipy import stats

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

        underflow = simstats['underflow']/simstats['cycles']
        # if (simstats['packets'] != 0):
        #     #underflow = simstats['pktinacc']
        #     underflow = simstats['pktinacc']/simstats['packets']
        # else:
        #     underflow = [0]

        # underflow = simstats['inacc']
        underflows.append(underflow[0])

    print(sorted(underflows))
    res = stats.ecdf(underflows)
    print(sorted(res.cdf.quantiles))

    res.cdf.plot(axs)

    # print(sorted(underflows))
    # print(underflows)
    axs.set_title("CDF Underflow (Snapshots [120, 130])")
    axs.set_xlabel("Ratio of Cycles Underflow")
    axs.set_ylabel("Cumulative Probability")
    # axs.plot(underflows)

    plt.savefig(args.outfile, bbox_inches="tight", dpi=500)
