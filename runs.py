#!/usr/bin/env python
# encoding: utf-8

"""
Runs all the protocols for all the traces provided.
"""

import os
import subprocess

protocols = ("MI", "MSI", "MESI", "MOSI", "MOESI", "MOESIF")

prog = "./sim_trace"
trace_dir = "./traces"
output_dir = "./runs"
run_dir = os.path.join(output_dir, "runs")
data_dir = os.path.join(output_dir, "data")

def run_simulation(trace, protocol):
    input_trace = os.path.join(trace_dir, trace)
    output_filename = "{}_{}.run".format(trace, protocol)
    output = os.path.join(run_dir, output_filename)
    output_file = open(output, "w+")
    cmd = (prog, "-t", input_trace, "-p", protocol)
    print "Running", " ".join(cmd)
    subprocess.call(cmd, stderr=subprocess.STDOUT, stdout=output_file)
    output_file.seek(0)
    return output_file

def extract_int_from_results(results_line):
    return int(results_line.split(":")[1].strip().split()[0])

def parse_results(trace, protocol, results_file):
    summary_lines = ("Run Time", "Cache Misses", "Cache Accesses",
                     "Silent Upgrades", "$-to-$ Transfers")
    data = {}
    for line in results_file:
        for s in summary_lines:
            if line.startswith(s):
                data[s] = extract_int_from_results(line)

    data_filename = "{}.csv".format(trace)
    data_filename = os.path.join(data_dir, data_filename)
    data_file = open(data_filename, "a")
    data_file.write(", ".join((protocol,
                               str(data["Run Time"]),
                               str(data["Cache Misses"]),
                               str(data["Cache Accesses"]),
                               str(data["Silent Upgrades"]),
                               str(data["$-to-$ Transfers"])
                              ))
                   )
    data_file.write("\n")

if __name__ == '__main__':
    try:
        os.mkdir(run_dir)
    except OSError:
        pass
    try:
        os.mkdir(data_dir)
    except OSError:
        pass

    traces = [os.path.join(trace_dir, d) for d in os.listdir(trace_dir)]
    traces = sorted(filter(os.path.isdir, traces))
    trace_names = [os.path.basename(d) for d in traces]
    for trace in trace_names:
        for protocol in protocols:
            results_file = run_simulation(trace, protocol)
            parse_results(trace, protocol, results_file)
