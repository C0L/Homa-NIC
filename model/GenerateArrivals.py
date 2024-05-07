import random
import math
import copy
import yaml
import argparse
import numpy as np
import matplotlib.pyplot as plt
import numpy as np
import scipy.stats

import statistics
import sys


def parseWorkload(path):
    w = []
    file  = open(path, 'r')
    lines = file.readlines()
        
    for line in lines:
           w.append(int(line.split()[0]))

    return w 

if __name__ == '__main__':

    parser = argparse.ArgumentParser(
                    prog='Generate Arrivals',
                    description='Generate arrivals under a given distribution for a given utilization ')

    parser.add_argument('-d', '--dist', required=True)
    parser.add_argument('-u', '--util', required=True)
    parser.add_argument('-w', '--workload', required=True)
    parser.add_argument('-a', '--arrival', required=True)
    parser.add_argument('-s', '--samples', required=True)
    # parser.add_argument('-u', '--utils', help='list of utilizations', type=str, required=True)

    args = parser.parse_args()

    # args = parser.parse_args()
    # utils = [float(item) for item in args.utils.split(',')]

    w = parseWorkload(args.workload)

    for rho in [float(args.util)]:
        ofile = open(args.arrival, "w")

        mst = np.mean(w)

        # mu  = 1/mst
        # lamda = rho * mu
        # arrival = lamda

        arrival = mst / rho
        a = (1+arrival)/arrival

        # Poisson arrival times + poisson packet sizes for outgoing messages
        t = 0
        for i in range(int(args.samples)):
            t += scipy.stats.lomax.rvs(a)
            # t += random.expovariate(arrival)

            ofile.write(f"{round(t)}\n")

    
