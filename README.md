CS 6290 Project 3
=================
Cache Coherence Protocols
-------------------------
## Sam Britt

- To build, type `make`. Running `make clean` will remove the build
  files.
- To run, type

    ./sim_trace -t <trace_directory> -p <protocol>

  where `<trace_directory>` is the directory containing the trace
  files, and `<protocol>` is one of the available cache coherency
  protocols (one of MI, MSI, MESI, MOSI, MOESI, and MOESIF). Results
  are output to stderr.
- The report source is in the `report/` directory. The pdf output has
  been sym-linked to the project root.
- To run the validation script, type

    ./validate.sh <protocols>

  where `<protocols>` is one or more protocols that you would like to
  validate. If `<protocols>` is not specified, then the default is all
  available protocols: MI, MSI, MESI, MOSI, MOESI, and MOESIF. The
  script will run the simulation for each of the validation input
  traces provided (that is, each of 4, 6, and 16 processors) for each
  of the protocols listed, and output a `diff` of the simulation
  output and the sample validation output. The simulation output for
  protocol `PROT` with `n` processors will also be saved in a file
  `runs/validation/trace_PROT_n`. This directory currently contains
  the validated output.
- The script `runs.py` will run the simulator on all the traces, and
  put the results in files in the `runs/runs/` directory. It will also
  parse the output, and put the processed results in csv files in the
  `runs/data/` directory.
- The script `plots.py` script will read the csv files in `runs/data/`
  and plot the relevant bar charts to files in the `runs/plots/`
  directory.
