#!/bin/bash

WORKLOADS=( w3 )
# WORKLOADS=( w1 w2 w4 )
# WORKLOADS=( w1 w2 w4 )
# WORKLOADS=( w1 w2 w3 w4 w5 )
UTILS=( 1.2 2 5 )
# MAXS=( 1 2 8 16 32 64 128 256 512 1024 2048 4098 8192 16384 )
# MAXS=( 1 32 64 96 128 160 192 224 256 512 1024 2048 )

COMPS=100
# COMPS=10000000

TRACEDIR=pifo_insertion_pulse

mkdir $TRACEDIR | :

for wk in "${WORKLOADS[@]}"; do
    for util in "${UTILS[@]}"; do
	priority=16
	# for max in "${MAXS[@]}"; do
	for i in {1..64}; do
	    max=$(($i * 32))
	    ./qsim --type pifo_insertion_pulse \
		   --priorities ${priority}   \
		   --workload ${wk}           \
		   --utilization ${util}      \
		   --comps ${COMPS} \
	           --max ${max} \
    		   --trace-file ${TRACEDIR}/${wk}_${util}_${max} &
	done
	# wait
    done
done

wait
