#!/bin/bash

WORKLOADS=( w1 w2 w3 w4 w5 )
ARRIVAL=( poisson )
UTILS=( .9 )

COMPS=100000
      
TRACEDIR=poisson_strict

mkdir $TRACEDIR | :

for wk in "${WORKLOADS[@]}"; do
    for util in "${UTILS[@]}"; do
	# for j in $(seq 0 10000 10000000); do
    	    ./sim_poisson_strict --workload ${wk} \
				 --priorities 10 \
				 --utilization ${util} \
				 --comps ${COMPS}      \
    	    			 --trace-file ${TRACEDIR}/${wk}_${util}
	# done
    done
done
