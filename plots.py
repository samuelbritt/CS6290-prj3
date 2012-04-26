#!/usr/bin/env python
# encoding: utf-8

import os
import pprint
import matplotlib.pyplot as plt
from scipy import *

data_dir = "./runs/data"
plots_dir = "./runs/plots"
protocols = ("MI", "MSI", "MESI", "MOSI", "MOESI", "MOESIF")

def read_data(data_file):
    data = {}
    for line in data_file:
        vals = line.split(", ")
        protocol = vals[0]
        run = {}
        run["runtime"]   = int(vals[1])
        run["misses"]    = int(vals[2])
        run["accesses"]  = int(vals[3])
        run["upgrades"]  = int(vals[4])
        run["transfers"] = int(vals[5])
        data[protocol] = run
    return data

def plot_runtimes(ax, data, protocol, indices, **kwargs):
    runs = [data[r] for r in sorted(data.keys())]
    runtimes = [r[p]["runtime"] for r in runs]
    return ax.bar(indices, runtimes, **kwargs)

if __name__ == '__main__':
    try:
        os.mkdir(plots_dir)
    except OSError:
        pass

    data_files = os.listdir(data_dir)
    data = {}
    for f in data_files:
        data[f] = read_data(open(os.path.join(data_dir, f)))

    f = plt.figure()
    ax = f.add_subplot(111)
    width = 0.8 / len(protocols)
    ind = range(len(data_files))
    colors = "krgbmc"
    bars = []
    for i, p in enumerate(protocols):
        bar = plot_runtimes(ax, data, p, ind, width=width,
                            color=colors[i % len(colors)])
        bars.append(bar[0])
        ind = [i + width for i in ind]
    ax.legend(bars, protocols)
    plt.show()
