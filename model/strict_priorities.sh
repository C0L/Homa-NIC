#!/bin/bash

WORKLOADS=( w1 w2 w3 )
ARRIVAL=( poisson )
UTILS=( .9 )

COMPS=1000000
      
TRACEDIR=strict_priorities_weibull

mkdir $TRACEDIR | :

for wk in "${WORKLOADS[@]}"; do
    for util in "${UTILS[@]}"; do

    	# ./sim_strict_priorities --workload ${wk} \
	# 			--priorities 100 \
	# 			--utilization ${util} \
	# 			--comps ${COMPS}      \
    	#     			--trace-file ${TRACEDIR}/${wk}_${util}_100


    	# ./sim_strict_priorities --workload ${wk} \
	# 			--priorities 1000000 \
	# 			--utilization ${util} \
	# 			--comps ${COMPS}      \
    	#     			--trace-file ${TRACEDIR}/${wk}_${util}_1000000

	# exit
	
	for i in {0..100}; do
	    priority=$(( $i*10 + 1))
    	    ./sim_strict_priorities --workload ${wk} \
				    --priorities ${priority} \
				    --utilization ${util} \
				    --comps ${COMPS}      \
    	    			    --trace-file ${TRACEDIR}/${wk}_${util}_${priority} &

	    if (( $i % 128 == 127 )) ; then
		wait
	    fi
	done
    done
done
