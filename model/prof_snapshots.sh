#!/bin/bash

WORKLOADS=( w1 w2 w3 )
UTILS=( .9 .9999 )
PRIORITIES=( 8 16 )

COMPS=1000000
# COMPS=100000000

SRCDIR=gen_snapshots
TRACEDIR=prof_snapshots

mkdir $TRACEDIR | :

for wk in "${WORKLOADS[@]}"; do
    for util in "${UTILS[@]}"; do
	for priority in "${PRIORITIES[@]}"; do
	    for ts in $(seq 0 10000 ${COMPS}); do
		./qsim --type pifo_naive_snapshot \
		       --priorities ${priority}   \
		       --workload ${wk}           \
		       --utilization ${util}      \
		       --snapshot ${SRCDIR}/${wk}_${util}_${priority}_${ts}.snap \
    		       --trace-file ${TRACEDIR}/${wk}_${util}_${priority}_${ts}
	    done
	done
    done
done
