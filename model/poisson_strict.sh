#!/bin/bash

WORKLOADS=( w1 w2 w3 w4 w5 )
ARRIVAL=( poisson )
UTILS=( .9999 )

COMPS=10000000
      
TRACEDIR=poisson_strict

mkdir $TRACEDIR | :

for wk in "${WORKLOADS[@]}"; do
    for util in "${UTILS[@]}"; do
	for i in {0..1000}; do
	    priority=$(( $i*10 + 1))
    	    ./sim_strict --workload ${wk} \
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
