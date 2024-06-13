#!/bin/bash

WORKLOADS=( w1 w2 w3 w4 w5 )
ARRIVAL=( poisson )
UTILS=( .9 )
# UTILS=( .8 .9 .99 )

COMPS=1000000
      
TRACEDIR=poisson_hw_model_traces

mkdir $TRACEDIR | :

for wk in "${WORKLOADS[@]}"; do
    for util in "${UTILS[@]}"; do
    	./sim_poisson_hw_model --workload ${wk} \
			    --utilization ${util} \
			    --comps ${COMPS}      \
    	    		    --trace-file ${TRACEDIR}/${wk}_${util}
    done
done
