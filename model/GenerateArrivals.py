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
import struct

from pathlib import Path

if __name__ == '__main__':

    parser = argparse.ArgumentParser(
                    prog='Generate Arrivals',
                    description='Generate arrivals under a given distribution for a given utilization ')

    parser.add_argument('-d', '--dist', required=True)
    parser.add_argument('-u', '--util', required=True)
    parser.add_argument('-w', '--workload', required=True)
    parser.add_argument('-a', '--arrival', required=True)
    parser.add_argument('-s', '--samples', required=True)

    args = parser.parse_args()

    ifile = open(args.workload, "r")
    
    w = np.fromfile(ifile, dtype=np.uint32)
    rho = float(args.util)

    mst = np.mean(w)
    arrival = mst * (1/rho)
    print("Distribution: " + args.dist)
    print("Mean Service Time: " + str(mst))
    print("Desired Arrival: " + str(arrival))

    if args.dist == "poisson":
        # print("Statistical Arrival: " + str(scipy.stats.poisson.stats(arrival, moments='mv')))
        ts = scipy.stats.poisson.rvs(1, size = int(args.samples)).astype(np.float32)

        ts = (ts / np.mean(ts)) * arrival
    elif args.dist == "lomax":
        ts = np.random.weibull(a=.1, size=int(args.samples)).astype(np.float32)

        print(np.mean(ts))
        # print((np.max(ts) * 5) / 1000)

        ts = (ts / np.mean(ts)) * arrival

        print(np.min(ts))
        print(np.max(ts))

        print("Sampled Arrival: " + str(np.mean(ts)))
        # print("Statistical Arrival: " + str(scipy.stats.lomax.stats(a, scale=scale, moments='mv')))
    elif args.dist == "weibull":
        ts = np.random.weibull(a=10, size=int(args.samples))
        print(np.mean(ts))
        ts = ts * arrival
        # print(ts[0:40])

        ts = ts.astype(np.float32)

        print("Sampled Arrival: " + str(np.mean(ts)))

    print("Sanity Check: Average Message Length/Average Inter Arrival Time = " + str(mst / np.mean(ts)))

    ts.tofile(args.arrival)
