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

    # scale = 1


    # TODO can compute a scale based on the workload to keep variance the same...

    ifile = open(args.workload, "r")
    
    w = np.fromfile(ifile, dtype=np.uint32)
    # print(w)
    for rho in [float(args.util)]:
        print(rho)
        print(w)
        mst = np.mean(w)
        print("Compute Mean: " + str(mst))

        # mu  = 1/mst
        # lamda = rho * mu

        # Normalized completition time

        # arrival = lamda

        arrival = mst * (1/rho)
        scale = arrival + 1

        # print(arrival)
        a = (scale+arrival)/arrival

        mst = 1 / (a-1)

        print(a)

        # print(scipy.stats.lomax.stats(a, moments='mv'))
        # print(scipy.stats.pareto.stats(a, moments='mv'))
        # Poisson arrival times + poisson packet sizes for outgoing messages
        # ts = np.zeros(int(args.samples), dtype=np.float32)
        # ts = np.random.pareto(a, size=int(args.samples)).astype(np.float32)

        ts = scipy.stats.lomax.rvs(a, scale=scale, size = int(args.samples)).astype(np.float32)

        print("Desired Arrival: " + str(arrival))
        print("Sampled Arrival: " + str(np.mean(ts)))
        print("Statistical Arrival: " + str(scipy.stats.lomax.stats(a, moments='mv')))
        print("Sanity Check: Average Message Length/Average Inter Arrival Time = " + str(mst / np.mean(ts)))

        # ts = scipy.stats.poisson.rvs(arrival, size = int(args.samples)).astype(np.float32)
        print(ts)

            # t += random.expovariate(arrival)
            # t += scipy.stats.poisson.rvs(1/arrival)
            # t += scipy.stats.poisson.rvs(1/arrival)

        ts.tofile(args.arrival)
        # ofile.write(round(t).to_bytes(8, byteorder='little', signed=False))

    
