#!/bin/bash

WORKLOADS=( w4 )
UTILS=( .9999 )
PRIORITIES=( 16 )

# COMPS=10000000
# COMPS=1000000
# COMPS=100000000

SRCDIR=gen_snapshots
TRACEDIR=prof_snapshots

mkdir $TRACEDIR | :

for wk in "${WORKLOADS[@]}"; do
    for util in "${UTILS[@]}"; do
	for priority in "${PRIORITIES[@]}"; do
	    # for ts in $(seq 0 1 10000); do
	    for ts in {0..1000}; do
		./qsim --type pifo_naive_snapshot \
		       --priorities ${priority}   \
		       --workload ${wk}           \
		       --utilization ${util}      \
		       --snapshot ${SRCDIR}/${wk}_${util}_${priority}_${ts}.snap \
    		       --trace-file ${TRACEDIR}/${wk}_${util}_${priority}_${ts} &
	    done
	done
    done
done
