#!/bin/bash

WORKLOADS=( w1 w2 w3 w4 w5 )
ARRIVAL=( weibull )

UTILS=( .7 .8 .9 )
      
COMPS=10000000

TRACEDIR=pareto_efficient_weibull_pifo_logn_traces

mkdir $TRACEDIR | :

for wk in "${WORKLOADS[@]}"; do
    for id in "${!UTILS[@]}"; do
	burst="${BURST[id]}"
	util="${UTILS[id]}"

	arrival=weibull

	for j in $(seq 0 100 2000); do
	    sl=$j
	    bs=8

	    hw=-1
	    lw=-1


	    ./sim_pifo_logn --workload ${wk} \
			    --utilization ${util} \
			    --comps ${COMPS}                                                     \
	    		    --trace-file ${TRACEDIR}/${wk}_${util}_${hw}_${lw}_${bs}_0_${sl} \
	    		    --high-water ${hw}                                                   \
	    		    --low-water  ${lw}                                                   \
	    		    --block-size ${bs}                                                   \
	    		    --sort-latency ${sl} &


	    # for i in {0..1024}; do
	    for i in {0..128}; do
		hw=$(( $i*64 + 32 ))
		lw=$(( $i*64 - 16 + 32))

	    	./sim_pifo_logn --workload ${wk} \
				--utilization ${util} \
				--comps ${COMPS}                                                     \
	    			--trace-file ${TRACEDIR}/${wk}_${util}_${hw}_${lw}_${bs}_0_${sl} \
	    			--high-water ${hw}                                                   \
	    			--low-water  ${lw}                                                   \
	    			--block-size ${bs}                                                   \
	    			--sort-latency ${sl} &

		if (( $i % 128 == 0 )) ; then
		    wait
		fi
	    done
	done
    done
done
