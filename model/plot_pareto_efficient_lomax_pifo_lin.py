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
import re
import os
import pandas as pd
import glob
from pathlib import Path


def parsefn(fn):
    fsplit = Path(fn).stem.split('_')
    return {
        'workload' : os.path.basename(fsplit[0]), 
        'util'     : float(fsplit[1]), 
        'hw'       : int(fsplit[2]),
        'lw'       : int(fsplit[3]),
        'bs'       : int(fsplit[4]),
        'cl'       : float(fsplit[5]),
        'sl'       : float(fsplit[6]),
        # 'burst'    : int(fsplit[7]),
        'stat'     : np.fromfile(fn, dtype=simstat_t, count=1)
    }

# TODO create some matplotlib defaults here

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        prog='Plot',
        description='Plot traces')

    parser.add_argument('-t', '--traces', type=str, nargs='+', required=True)
    parser.add_argument('-f', '--outfile', type=str, required=True)
    parser.add_argument('-w', '--workload', type=str, required=True)
    parser.add_argument('-u', '--util', type=str, nargs='+', required=True)

    args = parser.parse_args()

    fig, axs = plt.subplots()

    fig.tight_layout()

    simstat_t = np.dtype([('highwater', np.uint64),
                          ('lowwater', np.uint64),
                          ('max', np.uint64),
                          ('events', np.uint64),
                          ('compcount', np.uint64),
                          ('compsum', np.uint64),
                          ('cycles', np.uint64)])

    workload = args.workload

    color = iter(cm.rainbow(np.linspace(0, 1, 6)))
    c = next(color)

    for util in args.util:
        util = float(util)

        df = pd.DataFrame({
            'workload' : [],
            'util'     : [],
            'hw'       : [],
            'lw'       : [],
            'bs'       : [],
            'cl'       : [],
            'sl'       : [],
            'stat'     : []
        })

        traces = glob.glob(args.traces[0] + f'{workload}*{util}*.slotstats')

        for trace in traces:
            df.loc[len(df)] = parsefn(trace)

        samples = df.loc[(df['workload'] == workload)
                         & (df['hw'] != -1)
                         & (df['util'] == util)]

        gold = df.loc[(df['workload'] == workload)
                      & (df['hw'] == -1)
                      & (df['util'] == util)].iloc[0]

        mctgold  = (gold['stat']['compsum'] / gold['stat']['compcount'])[0]

        print(mctgold)

        origs = pd.DataFrame({
            'workload' : [],
            'util'     : [],
            'hw'       : [],
            'lw'       : [],
            'bs'       : [],
            'cl'       : [],
            'sl'       : [],
            'stat'     : []
        })

        # Remove non-convergent
        for i, orig in samples.iterrows():
            print(orig)
            mctorig = (orig['stat']['compsum'] / orig['stat']['compcount'])[0]

            # print(mctorig)

            if (mctgold/mctorig >= .99):
                origs.loc[len(origs)] = orig

        stripped = origs.copy()

        print(stripped)
        for i0, orig in origs.iterrows():
            stripped = stripped.loc[(stripped['hw'] <= orig['hw']) | (stripped['sl'] > orig['sl'])]

        print(stripped)

        wk = int(re.findall(r'\d+', workload)[0])-1
        axs.plot(stripped['hw'], stripped['sl'], color=c, label=str(util))
                           
        c = next(color)
    
    axs.text(.05, .05, workload, c='r', horizontalalignment='center', verticalalignment='center', transform = axs.transAxes)
    axs.set_xlabel("Small Queue Size")
    axs.set_ylabel(r"Large Queue Sorting Constant C ($\mu s$)")
    axs.set_title(r"PIFO Instance with $C * n$ Bulk Sort")

    yticks = []
    for t in (axs.get_yticklabels()):
        yticks.append((t.get_unitless_position()[1]*5)/1000)

    axs.set_yticklabels(yticks)

    handles, labels = axs.get_legend_handles_labels()

    newLabels, newHandles = [], []
    for handle, label in zip(handles, labels):
        if label not in newLabels:
            newLabels.append(label)
            newHandles.append(handle)

    axs.legend(newHandles, newLabels, loc='upper center', bbox_to_anchor=(0.5, -0.1),
               fancybox=False, shadow=False, ncol=2, title='Tail Index')

    plt.savefig(args.outfile, bbox_inches="tight")
