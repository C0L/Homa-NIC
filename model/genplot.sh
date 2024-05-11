#!/bin/bash

# WORKLOADS=( w1 w2 w3 w4 w5 )
# UTILS=( .9 .7 .5 .3 .1 )
# CYCLES=10000000


# Plot MCT vs Utilization for each of the 5 workloads

for wk in "${WORKLOADS[@]}";
do
    python3 Plot.py -t traces/${wk}_*_fifo.trace -f mm1_${wk}_fifo.png
    python3 Plot.py -t traces/${wk}_*_srpt.trace -f mm1_${wk}_srpt.png 
done
