#!/bin/bash

WORKLOADS=( w1 )
UTILS=( 5 )
# WORKLOADS=( w1 w2 )
# UTILS=( 1.1 1.2 1.3 1.4 1.5 1.6 1.7 1.8 1.9 )
# UTILS=( 1.2 2 5 )

COMPS=100

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
	    wait
	    exit
	done
	# wait
    done
done

wait
