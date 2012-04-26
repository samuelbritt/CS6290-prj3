CS 6290 Project 3
=================
Cache Coherence Protocols
-------------------------
## Sam Britt

- To build, type `make`.
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
