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


def parseWorkload(path):
    w = []
    
    # lines = file.readlines()
        
    # for line in lines:
        # ints = struct.unpack('iiii', data[:16])
        # w.append(int(line.split()[0]))

    return ints

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

    scale = 1

    # w = parseWorkload(args.workload)

    # data = Path(args.workload).read_bytes()
    # w = np.mean(struct.unpack('<Q', data))

    ifile = open(args.workload, "r")
    w = np.mean(np.fromfile(ifile, dtype=np.uint64))

    for rho in [float(args.util)]:
        ofile = open(args.arrival, "wb")

        mst = np.mean(w)

        # mu  = 1/mst
        # lamda = rho * mu
        # arrival = lamda

        arrival = mst * (1/rho)
        a = (scale+arrival)/arrival
        # # print("a value: " + str(a))

        # artest = 10 * 1
        # atest = (1+artest)/artest

        # for i in range(10000000):
        #     samp.append(scipy.stats.lomax.rvs(atest))

        # print("MEAN " + str(np.mean(samp)))

        # Poisson arrival times + poisson packet sizes for outgoing messages
        t = 0
        for i in range(int(args.samples)):
            t += scipy.stats.lomax.rvs(a, scale=scale)
            # t += random.expovariate(arrival)
            # t += scipy.stats.poisson.rvs(1/arrival)
            # t += scipy.stats.poisson.rvs(1/arrival)

            ofile.write(round(t).to_bytes(8, byteorder='little', signed=False))

    
