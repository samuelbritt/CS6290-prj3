#!/usr/bin/env python
# encoding: utf-8

import os
import pprint
from matplotlib import rc
import matplotlib.pyplot as plt
from scipy import *

rc('text', usetex=True)
rc('font', family='serif', size=9.0)
rc('legend', fontsize='small')
rc('xtick.major', size=1)
rc('ytick.major', size=1)
rc('figure.subplot', bottom=.2)
rc('savefig', dpi=300)
rc('axes', linewidth=.5)

data_dir = "./runs/data"
plots_dir = "./runs/plots"
all_protocols = ("MI", "MSI", "MOSI", "MESI", "MOESI", "MOESIF")
colors = {}

def init_colors():
    all_colors = "krgbmc"
    for i, p in enumerate(all_protocols):
        colors[p] = all_colors[i % len(all_colors)]

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

def plot_data(ax, data, runs, protocol, indices, data_type, **kwargs):
    run_data = [data[r] for r in runs]
    runtimes = [r[protocol][data_type] for r in run_data]
    return ax.bar(indices, runtimes, label=protocol, **kwargs)

def plot_all_data(ax, data, data_filenames, data_filelabels, data_type,
                  protocols):
    width = 0.8 / len(all_protocols)
    ind = arange(len(data_filenames)) + (1 - (len(protocols) * width))/2
    edgecolor="white"
    linewidth=0
    bars = []
    for i, p in enumerate(protocols):
        color=colors[p]
        indices = ind + i*width
        bar = plot_data(ax, data, data_filenames, p,
                        indices, data_type=data_type,
                        width=width, color=color, edgecolor=edgecolor,
                        lw=linewidth
                       )
        bars.append(bar[0])
    ax.set_xlim(0, len(data_filenames))
    ax.set_xticks(ind + (len(protocols) * width)/2)
    ax.set_xticklabels(data_filelabels,
                       horizontalalignment='right', rotation=30)
    return bars

def make_split_subplot(data, data_filenames, data_filelabels, data_type,
                       ylabel=None, protocols=all_protocols):
    f = plt.figure(figsize=(4.5,3))
    ax1 = plt.subplot2grid((1,12), (0,0), colspan=3)
    ax2 = plt.subplot2grid((1,12), (0,4), colspan=8)
    bars1 = plot_all_data(ax1, data, data_filenames[:3], data_filelabels[:3],
                          data_type=data_type, protocols=protocols)
    bars2 = plot_all_data(ax2, data, data_filenames[3:], data_filelabels[3:],
                          data_type=data_type, protocols=protocols)
    ax1.set_ylabel(ylabel)
    ax2.legend(bars2, protocols, "best", frameon=False)
    return f

if __name__ == '__main__':
    try:
        os.mkdir(plots_dir)
    except OSError:
        pass

    data_files = [
        ("4proc_validation.csv"  , "4 Processor"),
        ("8proc_validation.csv"  , "8 Processor"),
        ("16proc_validation.csv" , "16 Processor"),
        ("trace1.csv"            , "Trace 1"),
        ("trace2.csv"            , "Trace 2"),
        ("trace3.csv"            , "Trace 3"),
        ("trace4.csv"            , "Trace 4"),
        ("trace5.csv"            , "Trace 5"),
        ("trace6.csv"            , "Trace 6"),
        ("trace7.csv"            , "Trace 7"),
        ("trace8.csv"            , "Trace 8"),
    ]

    data_filenames = [f[0] for f in data_files]
    data_filelabels = [f[1] for f in data_files]

    init_colors()

    data = {}
    for f in data_filenames:
        data[f] = read_data(open(os.path.join(data_dir, f)))

    f = make_split_subplot(data, data_filenames, data_filelabels,
                           data_type="runtime", ylabel="Runtime (cycles)")
    for ax in f.get_axes():
        ax.ticklabel_format(style='sci', scilimits=(0,0), axis='y')
    f.savefig(os.path.join(plots_dir, "runtime.pdf"))

    f = make_split_subplot(data, data_filenames, data_filelabels,
                           data_type="transfers",
                           ylabel="Cache to Cache Transfers")
    for ax in f.get_axes():
        ax.ticklabel_format(style='sci', scilimits=(0,0), axis='y')
    f.savefig(os.path.join(plots_dir, "transfers.pdf"))

    f = make_split_subplot(data, data_filenames, data_filelabels,
                           data_type="upgrades", ylabel="Silent Upgrades",
                           protocols=("MESI", "MOESI", "MOESIF"))
    f.savefig(os.path.join(plots_dir, "upgrades.pdf"))

    f = make_split_subplot(data, data_filenames, data_filelabels,
                           data_type="misses", ylabel="Cache Misses")
    for ax in f.get_axes():
        ax.ticklabel_format(style='sci', scilimits=(0,0), axis='y')
    f.savefig(os.path.join(plots_dir, "misses.pdf"))

    f = make_split_subplot(data, data_filenames, data_filelabels,
                           data_type="accesses", ylabel="Cache Accesses")
    for ax in f.get_axes():
        ax.ticklabel_format(style='sci', scilimits=(0,0), axis='y')
    f.savefig(os.path.join(plots_dir, "accesses.pdf"))

    plt.show()
