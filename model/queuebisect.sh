#!/bin/bash

WORKLOADS=( w1 w2 w3 w4 w5 )
ARRIVAL=( poisson )
UTILS=( .5 .6 .7 .8 .9 .99 .9999)

# Pairs of high/low water marks
HWS=( -1 40 40 30 20 10 )
LWS=( -1 30 30 20 10 0 ) # Based on reasonable PIFO sizes
CL=( 0 0 140 140 140 140 ) # Communication latency
SL=( 0 0 40 40 40 40 ) # Sort latency
BS=( 0 0 5 5 5 5 ) # Block size to transfer between queues

CYCLES=10000000

for wk in "${WORKLOADS[@]}"; do
    if [ ! -e "dists/${wk}_lengths" ]; then
	./distgen $wk ${CYCLES} dists/${wk}_lengths
    fi

    for util in "${UTILS[@]}"; do
	for arrival in "${ARRIVAL[@]}"; do
	    if [ ! -e "dists/${wk}_${util}_${arrival}_arrivals" ]; then
    		echo $util
		python3.10 GenerateArrivals.py \
			-d ${arrival} \
			-u $util \
			-w dists/${wk}_lengths \
			-a dists/${wk}_${util}_${arrival}_arrivals \
			-s ${CYCLES}
	    fi
	    for wm in "${!HWS[@]}"; do
		hw="${HWS[wm]}"
		lw="${LWS[wm]}"
		cl="${CL[wm]}"
		sl="${SL[wm]}"
		bs="${BS[wm]}"
    		./simulator --queue-type SRPT                            \
			    --length-file dists/${wk}_lengths            \
			    --arrival-file dists/${wk}_${util}_${arrival}_arrivals  \
			    --cycles ${CYCLES}                           \
			    --trace-file traces/${wk}_${util}_${hw}_${lw}_${bs}_${cl}_${sl}_${arrival}_srpt \
			    --high-water ${hw}                           \
			    --low-water  ${lw}                           \
			    --chain-latency ${cl}                        \
			    --block-size ${bs}                           \
			    --sort-latency ${sl}
	    done
	done
    done
done
