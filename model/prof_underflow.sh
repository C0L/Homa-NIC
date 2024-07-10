#!/bin/bash

WORKLOADS=( w1 w4 )
UTILS=( .9999 )
PRIORITIES=( 16 )

COMPS=10000000
# COMPS=100000000

TRACEDIR=prof_underflow

mkdir $TRACEDIR | :

for wk in "${WORKLOADS[@]}"; do
    for util in "${UTILS[@]}"; do
	for priority in "${PRIORITIES[@]}"; do
	    ./qsim --type pifo_naive    \
		   --priorities ${priority}     \
		   --workload ${wk}       \
		   --utilization ${util}   \
		   --comps ${COMPS} \
    		   --trace-file ${TRACEDIR}/${wk}_${util}_${priority} &
	done
    done
done
