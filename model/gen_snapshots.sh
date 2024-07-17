#!/bin/bash

WORKLOADS=( w1 )
UTILS=( 1.2 2 5 )
# MAXS=( 64 128 256 512 )
MAXS=( 64 128 256 512 1024 2048 4098 8192 16384 )

# COMPS=100000
COMPS=10000000

TRACEDIR=gen_snapshots

mkdir $TRACEDIR | :

for wk in "${WORKLOADS[@]}"; do
    for util in "${UTILS[@]}"; do
	priority=16
	for max in "${MAXS[@]}"; do
	    ./qsim --type pifo_naive \
		   --priorities ${priority}   \
		   --workload ${wk}           \
		   --utilization ${util}      \
		   --comps ${COMPS} \
	           --max ${max} \
    		   --trace-file ${TRACEDIR}/${wk}_${util}_${max} &
	done
    done
done

wait
