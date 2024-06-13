#!/bin/bash

WORKLOADS=( w1 w2 w3 w4 w5 )
ARRIVAL=( poisson )
UTILS=( .7 .8 .9 )

# COMPS=10000000
COMPS=1000000000
      
TRACEDIR=poisson_prof_traces

mkdir $TRACEDIR | :

for wk in "${WORKLOADS[@]}"; do
    for util in "${UTILS[@]}"; do
    	./sim_poisson_ideal --workload ${wk} \
			    --utilization ${util} \
			    --comps ${COMPS}      \
    	    		    --trace-file ${TRACEDIR}/${wk}_${util} &
    done
done
