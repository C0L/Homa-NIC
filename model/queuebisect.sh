#!/bin/bash

# WORKLOADS=( w5 )
WORKLOADS=( w1 w2 w3 w4 w5 )
ARRIVAL=( poisson )
UTILS=( .1 .2 .3 .4 .5 .6 .7 .8 .9 .9999)
HWS=( 60 50 40 30 20 10 )
LWS=( 50 40 30 20 10 0 ) # Based on reasonable PIFO sizes
CL=( 0 0 0 0 0 0 0 ) # Communication latency
SL=( 0 0 0 0 0 0 0 ) # Sort latency
BS=( 5 5 5 5 5 5 5 ) # Block size to transfer between queues

COMPS=1000000
# COMPS=10000
CYCLES=100000000

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
			    --cycles ${COMPS}                           \
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
