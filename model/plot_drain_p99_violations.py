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
import os
from operator import itemgetter, attrgetter
from statsmodels.stats.weightstats import DescrStatsW

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
                    prog='Plot',
                    description='Plot traces')

    parser.add_argument('-t', '--traces', type=str, nargs='+', required=True)
    parser.add_argument('-f', '--outfile', type=str, required=True)

    args = parser.parse_args()

    fig, axs = plt.subplots(1)

    fig.tight_layout()

    inaccs = {}
    for trace in args.traces:
        s = os.path.basename(trace).split('_')
        print(s)

        if s[1] not in inaccs.keys():
            inaccs[s[1]] = []

        queuestats, simstats, slotstats = cfg.parse_stats(trace)

        print(slotstats)

        if len(slotstats) != 0:
            wq = DescrStatsW(data=slotstats, weights=slotstats)
            p = wq.quantile(probs=.99, return_pandas=False)
            # print(p)
            # p = (abs(slotstats-p).argmin())
            # inaccs[s[1]].append((int(s[2]), np.mean(slotstats)))
            inaccs[s[1]].append((int(s[2]), p))
        # print(p)
        # pp.append(p)
        # print(sum(occupied[p::])/sum(occupied))

    for arr in inaccs:
        # print(arr)
        # print(sorted(inaccs[arr], key=itemgetter(0)))
        x, y = zip(*sorted(inaccs[arr], key=itemgetter(0)))
        axs.plot(x, y, 'o', label=arr)

    # axs.set_xlim(0, 300)

    axs.legend(title='Utilization')
    axs.set_ylabel("p99 Degree of Violation ")
    axs.set_xlabel("Maximum Queue Size")
    axs.set_title("p99 Degree of Violation by Max Queue Size Size (" + s[0] + ")")
    plt.savefig(args.outfile, bbox_inches="tight")
    # plt.savefig(args.outfile, bbox_inches="tight", dpi=500)
