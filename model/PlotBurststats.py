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

def parsefn(fn):
    fn = fn.split('_')
    return {
        'workload' : fn[0], 
        'util'     : fn[1], 
        'hw'       : fn[2],
        'lw'       : fn[3],
        'bs'       : fn[4],
        'cl'       : fn[5],
        'sl'       : fn[6],
        'burst'    : fn[7]
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

    slotstat_t = np.dtype([('validcycles', np.uint64), ('totalbacklog', np.uint64), ('backlog', np.uint64), ('mintotalbacklog', np.uint64), ('minbacklog', np.uint64)])
    simstat_t = np.dtype([('highwater', np.uint64), ('lowwater', np.uint64), ('max', np.uint64), ('events', np.uint64), ('compcount', np.uint64), ('compsum', np.uint64), ('cycles', np.uint64)])

    # TODO need to make the mean compleititon time only for the burst

            # wk = int(re.findall(r'\d+', cfg['workload'])[0])-1
            # qt = f"lw:{cfg['lw']}, hw:{cfg['hw']}, bs:{cfg['bs']}, cl:{cfg['cl']}, cs:{cfg['sl']}"

            # simstats  = np.fromfile(trace, dtype=simstat_t, count=1)
            # slotstats = np.fromfile(trace, dtype=slotstat_t, count=-1, offset=simstats.nbytes)

    points = {}
    golden = {}

    for trace in args.traces:
        cfg = parsefn(trace)
        # print(cfg)
        
        if (int(cfg['hw']) == -1):
            # TODO also need workload
            golden[cfg['burst']+cfg['util']]=trace
        else:
            # TODO also need workload
            if cfg['burst']+cfg['util'] not in points:
                points[cfg['burst']+cfg['util']] = [trace]
            else:
                points[cfg['burst']+cfg['util']].append(trace)

    print(golden)

    doms = {}

    # Remove dominated points
    for pset in points:
        dom = []
        print(points[pset])
        for trace in points[pset]:
            cfgorig = parsefn(trace)
            statorig = np.fromfile(trace, dtype=simstat_t, count=1)

            mctorig  = (statorig['compsum'] / statorig['compcount'])[0]
            commorig = (statorig['highwater'] / statorig['cycles'])[0]

            # print(cfg)

            # wk = int(re.findall(r'\d+', cfg['workload'])[0])-1
            # qt = f"lw:{cfg['lw']}, hw:{cfg['hw']}, bs:{cfg['bs']}, cl:{cfg['cl']}, cs:{cfg['sl']}"

            add = True 
            for comp in points[pset]:
                cfgcmp = parsefn(comp)
                statscmp  = np.fromfile(comp, dtype=simstat_t, count=1)

                mctcmp  = (statscmp['compsum'] / statscmp['compcount'])[0]
                commcmp = (statscmp['highwater'] / statscmp['cycles'])[0]

                statsgold = np.fromfile(golden[cfgcmp['burst']+cfgcmp['util']], dtype=simstat_t, count=1)
                mctgold  = (statsgold['compsum'] / statsgold['compcount'])[0]

                if ((mctorig - mctgold)/((mctorig + mctgold)/2) > 1):
                    add = True 
                else:
                    # pass
                    # print("CHECK")
                    # print(cfgorig['hw'])
                    # print(cfgcmp['hw'])
                    # print(commorig)
                    # print(commcmp)

                    # diff = (commorig[0] - commcmp[0])/((commorig[0] + commcmp[0])/2)

                    # if (cfgorig['hw'] >= cfgcmp['hw'] and diff > .1):
                    # if ((cfgorig['hw'] > cfgcmp['hw'] and commorig > commcmp)):
                    if ((cfgorig['hw'] > cfgcmp['hw'] and commorig == commcmp) or (cfgorig['hw'] == cfgcmp['hw'] and commorig > commcmp) or (cfgorig['hw'] > cfgcmp['hw'] and commorig > commcmp)):
                        # add = False
                        add = True 
                        # print("DOMINATED")

            if (add):
                dom.append(trace)

        doms[cfgorig['burst']+cfgorig['util']] = dom

    # TODO still need to organize by workload


    color = iter(cm.rainbow(np.linspace(0, 1, 4)))
    c = next(color)

    cmap = {}

    for dom in doms:
        for efficient in doms[dom]:
            print(efficient)
            cfgorig = parsefn(efficient)
            statorig = np.fromfile(efficient, dtype=simstat_t, count=1)

            commorig = statorig['highwater'] / statscmp['cycles']

            axs[0].plot(int(cfgorig['hw']), commorig, 'o', color=c, label=cfgorig['util'] + ' , ' + cfgorig['burst'])

        c = next(color)

    for i in range(5):
        axs[i].text(.05, .05, 'w' + str(i+1), c='r', horizontalalignment='center', verticalalignment='center', transform = axs[i].transAxes)
        axs[i].set_xlabel("Min. Small Queue With Fidelity")
        axs[i].set_ylabel("Min. Communication Overhead")
        axs[i].set_ylim(axs[i].get_ylim()[::-1])
        axs[i].set_xlim(axs[i].get_xlim()[::-1])

    handles, labels = axs[0].get_legend_handles_labels()

    newLabels, newHandles = [], []
    for handle, label in zip(handles, labels):
        if label not in newLabels:
            newLabels.append(label)
            newHandles.append(handle)

    axs[4].legend(newHandles, newLabels, loc='upper center', bbox_to_anchor=(0.5, -0.1),
                  fancybox=False, shadow=False, ncol=2, title='Utilization')

    plt.savefig(args.outfile, bbox_inches="tight")
