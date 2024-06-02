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



def parsefn(fn):
    fsplit = fn.split('_')
    return {
        'workload' : os.path.basename(fsplit[0]), 
        'util'     : float(fsplit[1]), 
        'hw'       : int(fsplit[2]),
        'lw'       : int(fsplit[3]),
        'bs'       : int(fsplit[4]),
        'cl'       : float(fsplit[5]),
        'sl'       : float(fsplit[6]),
        'burst'    : int(fsplit[7]),
        'stat'     : np.fromfile(fn, dtype=simstat_t, count=1)
    }

# TODO create some matplotlib defaults here

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
                    prog='Plot',
                    description='Plot traces')

    parser.add_argument('-t', '--traces', type=str, nargs='+', required=True)
    parser.add_argument('-f', '--outfile', type=str, required=True)

    args = parser.parse_args()

    fig, axs = plt.subplots(5, figsize=(5,20))

    fig.tight_layout()

    simstat_t = np.dtype([('highwater', np.uint64),
                          ('lowwater', np.uint64),
                          ('max', np.uint64),
                          ('events', np.uint64),
                          ('compcount', np.uint64),
                          ('compsum', np.uint64),
                          ('cycles', np.uint64)])

    df = pd.DataFrame({
        'workload' : [],
        'util'     : [],
        'hw'       : [],
        'lw'       : [],
        'bs'       : [],
        'cl'       : [],
        'sl'       : [],
        'burst'    : [],
        'stat'     : []
        })

    traces = glob.glob(args.traces[0] + '*burst*.slotstats')
    print(traces)

    for trace in traces:
        print(trace)
        df.loc[len(df)] = parsefn(trace)

    print("COMPLETED PARSING")
    # doms = {}

    for workload in df['workload'].unique():
        print(workload)

        color = iter(cm.rainbow(np.linspace(0, 1, 4)))
        c = next(color)

        for burst in df['burst'].unique():
            print(burst)
            for util in df['util'].unique():
                print(util)
                samples = df.loc[(df['workload'] == workload)
                                 & (df['burst'] == burst)
                                 & (df['hw'] != -1)
                                 & (df['util'] == util)]

                gold = df.loc[(df['workload'] == workload)
                                 & (df['burst'] == burst)
                                 & (df['hw'] == -1)
                                 & (df['util'] == util)].iloc[0]

                mctgold  = (gold['stat']['compsum'] / gold['stat']['compcount'])[0]

                origs = pd.DataFrame({
                    'workload' : [],
                    'util'     : [],
                    'hw'       : [],
                    'lw'       : [],
                    'bs'       : [],
                    'cl'       : [],
                    'sl'       : [],
                    'burst'    : [],
                    'stat'     : []
                })

                # Remove non-convergent
                for i, orig in samples.iterrows():
                    mctorig = (orig['stat']['compsum'] / orig['stat']['compcount'])[0]
                    if (abs(mctgold - mctorig)/((mctgold + mctorig)/2) <= .01):
                        origs.loc[len(origs)] = orig

                stripped = origs.copy()

                for i0, orig in origs.iterrows():
                    stripped = stripped.loc[(stripped['hw'] <= orig['hw']) | (stripped['sl'] > orig['sl'])]


                wk = int(re.findall(r'\d+', workload)[0])-1
                axs[wk].plot(stripped['hw'], stripped['sl'], 'o', color=c, label=str(util) + ' , ' + str(burst))

            c = next(color)

    # for dom in doms:
    #     for efficient in doms[dom]:
    #         cfgorig = parsefn(efficient)
    #         statorig = np.fromfile(efficient, dtype=simstat_t, count=1)

    #         axs[0].plot(int(cfgorig['hw']), float(cfgorig['sl']), 'o', color=c, label=cfgorig['util'] + ' , ' + cfgorig['burst'])

    #     c = next(color)

    for i in range(5):
        axs[i].text(.05, .05, 'w' + str(i+1), c='r', horizontalalignment='center', verticalalignment='center', transform = axs[i].transAxes)
        axs[i].set_xlabel("Min. Queue Size")
        axs[i].set_ylabel("Min. Sorting Accuracy")
        axs[i].set_ylim(axs[i].get_ylim()[::-1])
        # axs[i].set_xlim(axs[i].get_xlim()[::-1])

    handles, labels = axs[0].get_legend_handles_labels()

    newLabels, newHandles = [], []
    for handle, label in zip(handles, labels):
        if label not in newLabels:
            newLabels.append(label)
            newHandles.append(handle)

    axs[4].legend(newHandles, newLabels, loc='upper center', bbox_to_anchor=(0.5, -0.1),
                  fancybox=False, shadow=False, ncol=2, title='Utilization')

    plt.savefig(args.outfile, bbox_inches="tight")


# for some burst size, we plot the maximum utilizations that can be sustained 
