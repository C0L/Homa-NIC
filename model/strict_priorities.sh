#!/bin/bash

WORKLOADS=( w1 w2 w3 w4 w5 )
UTILS=( .9999 )

PRIORITIES=( 1 2 4 8 16 32 64 128 256 )

COMPS=1000000
      
TRACEDIR=strict_priorities_poisson

mkdir $TRACEDIR | :

for wk in "${WORKLOADS[@]}"; do
    for util in "${UTILS[@]}"; do
	./sim_strict_priorities --workload ${wk} \
	     			--priorities 4 \
	     			--utilization ${util} \
	     			--comps ${COMPS}      \
    	     			--trace-file ${TRACEDIR}/${wk}_${util}_4

	./sim_strict_priorities --workload ${wk} \
	     			--priorities 8 \
	     			--utilization ${util} \
	     			--comps ${COMPS}      \
    	     			--trace-file ${TRACEDIR}/${wk}_${util}_8

	./sim_strict_priorities --workload ${wk} \
	     			--priorities 16 \
	     			--utilization ${util} \
	     			--comps ${COMPS}      \
    	     			--trace-file ${TRACEDIR}/${wk}_${util}_16

	./sim_strict_priorities --workload ${wk} \
	     			--priorities 32 \
	     			--utilization ${util} \
	     			--comps ${COMPS}      \
    	     			--trace-file ${TRACEDIR}/${wk}_${util}_32


	./sim_strict_priorities --workload ${wk} \
	     			--priorities 64 \
	     			--utilization ${util} \
	     			--comps ${COMPS}      \
    	     			--trace-file ${TRACEDIR}/${wk}_${util}_64

	./sim_strict_priorities --workload ${wk} \
	     			--priorities 128 \
	     			--utilization ${util} \
	     			--comps ${COMPS}      \
    	     			--trace-file ${TRACEDIR}/${wk}_${util}_128

	./sim_ideal --workload ${wk} \
	     	    --utilization ${util} \
	     	    --comps ${COMPS}      \
    	     	    --trace-file ${TRACEDIR}/${wk}_${util}_inf


	# for priority in "${PRIORITIES[@]}"; do
	    # priority=$(( $i*1 + 1))
    	    # ./sim_strict_priorities --workload ${wk} \
	    # 			    --priorities ${priority} \
	    # 			    --utilization ${util} \
	    # 			    --comps ${COMPS}      \
    	    # 			    --trace-file ${TRACEDIR}/${wk}_${util}_${priority} &

	    # if (( $i % 128 == 127 )) ; then
	    # 	wait
	    # fi
	# done
    done
done
