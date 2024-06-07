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
    print(fsplit)
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
    # fig, axs = plt.subplots(5, figsize=(5,20))

    fig.tight_layout()

    simstat_t = np.dtype([('highwater', np.uint64),
                          ('lowwater', np.uint64),
                          ('max', np.uint64),
                          ('events', np.uint64),
                          ('compcount', np.uint64),
                          ('compsum', np.uint64),
                          ('cycles', np.uint64)])

    print("COMPLETED PARSING")

    workload = args.workload
    print(workload)

    color = iter(cm.rainbow(np.linspace(0, 1, 6)))
    c = next(color)

    for util in args.util:
        print(util)
        util = float(util)

        df = pd.DataFrame({
            'workload' : [],
            'util'     : [],
            'hw'       : [],
            'lw'       : [],
            'bs'       : [],
            'cl'       : [],
            'sl'       : [],
            # 'burst'    : [],
            'stat'     : []
        })

        traces = glob.glob(args.traces[0] + f'{workload}*{util}*.slotstats')
        # traces = glob.glob(args.traces[0] + f'{workload}*{util}*burst*.slotstats')

        print(traces)

        for trace in traces:
            print(trace)
            df.loc[len(df)] = parsefn(trace)

        print(df)
        print(util)
        samples = df.loc[(df['workload'] == workload)
                         & (df['hw'] != -1)
                         & (df['util'] == util)]

        gold = df.loc[(df['workload'] == workload)
                      & (df['hw'] == -1)
                      & (df['util'] == util)].iloc[0]

        print(samples)

        print(gold)

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
            mctorig = (orig['stat']['compsum'] / orig['stat']['compcount'])[0]

            # print(mctorig)

            # origs.loc[len(origs)] = orig
            # if (mctgold/mctorig >= .99 and mctgold/mctorig <= 1.01):
            if (mctgold/mctorig >= .999):
                origs.loc[len(origs)] = orig
                #else:
                #    print("toss")
                #    if (mctgold/mctorig >= 1.00):
                #        print(mctgold/mctorig)

        stripped = origs.copy()

        for i0, orig in origs.iterrows():
            stripped = stripped.loc[(stripped['hw'] <= orig['hw']) | (stripped['sl'] > orig['sl'])]
        # stripped = stripped.loc[((stripped['hw'] <= orig['hw']) & (stripped['sl'] == orig['sl'])) |  (stripped['sl'] != orig['sl'])]
        # stripped = stripped.loc[(stripped['hw'] <= orig['hw']) | (stripped['sl'] > orig['sl'])]

        for i, orig in stripped.iterrows():
            print(orig)
            mctorig = (orig['stat']['compsum'] / orig['stat']['compcount'])[0]
            print(mctorig)

        wk = int(re.findall(r'\d+', workload)[0])-1
        axs.plot(stripped['hw'], stripped['sl'], 'o', color=c, label=str(util))
        # axs.plot(stripped['hw'], stripped['sl'], 'o', color=c, label=str(util))
                        
        # maxs = []
        #                 
        # for j, o in stripped.iterrows():
        #     maxs.append(o['stat']['max'])
                            
        # axs.plot(maxs, stripped['sl'], '^', color=c, label=str(util))
        # axs[wk].plot(stripped['hw'], stripped['sl'], 'o', color=c, label=str(util) + ' , ' + str(burst))
                            
        c = next(color)

    
    axs.text(.05, .05, workload, c='r', horizontalalignment='center', verticalalignment='center', transform = axs.transAxes)
    axs.set_xlabel("Small Queue Size")
    axs.set_ylabel("Large Queue Sorting Constant")
    # axs.set_ylim(axs.get_ylim()[::-1])
    # axs.set_yticks([1.0, 0.8, 0.6, 0.4, 0.2, 0.0])
    # axs.set_yticklabels([0.0, 0.2, 0.4, 0.6, 0.8, 1.0])

    # for i in range(5):
    #     axs[i].text(.05, .05, 'w' + str(i+1), c='r', horizontalalignment='center', verticalalignment='center', transform = axs[i].transAxes)
    #     # axs[i].set_xlim(xmax=20)
    #     axs[i].set_xlabel("Queue Size")
    #     axs[i].set_ylabel("Min. Sorting Accuracy")
    #     axs[i].set_ylim(axs[i].get_ylim()[::-1])
    # axs[i].set_xlim(axs[i].get_xlim()[::-1])

    handles, labels = axs.get_legend_handles_labels()

    newLabels, newHandles = [], []
    for handle, label in zip(handles, labels):
        if label not in newLabels:
            newLabels.append(label)
            newHandles.append(handle)

    axs.legend(newHandles, newLabels, loc='upper center', bbox_to_anchor=(0.5, -0.1),
               fancybox=False, shadow=False, ncol=2, title='Tail Index')

    plt.savefig(args.outfile, bbox_inches="tight")
