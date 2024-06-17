#!/bin/bash

WORKLOADS=( w1 w2 )
UTILS=( .9999 )

PRIORITIES=( 1 2 4 8 16 32 64 128 256 )

COMPS=1000000

TRACEDIR=hw_model_poisson

mkdir $TRACEDIR | :

for wk in "${WORKLOADS[@]}"; do
    for util in "${UTILS[@]}"; do
	# for priority in "${PRIORITIES[@]}"; do
	for i in {0..255}; do
	    priority=$(( $i*1 + 1))
    	    ./sim_hw_model --workload ${wk} \
				    --priorities ${priority} \
				    --utilization ${util} \
				    --comps ${COMPS}      \
    	    			    --trace-file ${TRACEDIR}/${wk}_${util}_${priority} &

	    # if (( $i % 128 == 127 )) ; then
	    # 	wait
	    # fi
	done
    done
done
