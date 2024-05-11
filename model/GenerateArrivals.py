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
        print("Statistical Arrival: " + str(scipy.stats.poisson.stats(arrival, moments='mv')))
        ts = scipy.stats.poisson.rvs(arrival, size = int(args.samples)).astype(np.float32)
    elif args.dist == "lomax":
        # Mean of 1
        a = 1.15
        scale = 1

        mean = scale/(a-1)

        # scale = arrival * (a-1)
        print("scale " + str(scale))
        # scale = 1
        # a = (scale+arrival)/arrival
        # 1 = (scale+arrival)/arrival
        # 1*arrival = scale + arrival


        ts = scipy.stats.lomax.rvs(a, scale=scale, size = int(args.samples)).astype(np.float32)
        print("Sampled Arrival: " + str(np.mean(ts)))
        ts = ts/mean * arrival
        print("Sampled Arrival: " + str(np.mean(ts)))


        #ts = scipy.stats.lomax.rvs(a, scale=scale, size = int(args.samples)).astype(np.float32)
        print("Statistical Arrival: " + str(scipy.stats.lomax.stats(a, scale=scale, moments='mv')))
        # print("Statistical Arrival: " + str(scipy.stats.lomax.stats(a, moments='mv')))

    print("Sampled Arrival: " + str(np.mean(ts)))
    print("Sanity Check: Average Message Length/Average Inter Arrival Time = " + str(mst / np.mean(ts)))

    ts.tofile(args.arrival)