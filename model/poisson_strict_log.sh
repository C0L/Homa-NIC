#!/bin/bash

WORKLOADS=( w1 w2 w3 w4 w5 )
ARRIVAL=( poisson )
UTILS=( .9 )

COMPS=100000
      
TRACEDIR=poisson_strict_log

mkdir $TRACEDIR | :

for wk in "${WORKLOADS[@]}"; do
    for util in "${UTILS[@]}"; do
	for i in {0..20}; do
	    priority=$(( $i*10 + 1))
    	    ./sim_poisson_strict_log --workload ${wk} \
				     --priorities ${priority} \
				     --utilization ${util} \
				     --comps ${COMPS}      \
    	    			     --trace-file ${TRACEDIR}/${wk}_${util}_${priority} &

	    if (( $i % 128 == 0 )) ; then
		wait
	    fi
	done
    done
done
