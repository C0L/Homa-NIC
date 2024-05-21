#!/bin/bash

WORKLOADS=( w1 w2 w3 w4 w5 )
# WORKLOADS=( w1 w2 w3 w4 w5 )
ARRIVAL=( lomax )
UTILS=( 1.1 1.2 1.3 1.4 1.5 1.6 1.7 )
HWS=( -1 20 40 80 160 320 640 1280 -1 20 40 80 160 320 640 1280 )
LWS=( -1 10 30 70 150 310 630 1270 -1 10 30 70 150 310 630 1270 ) # Based on reasonable PIFO sizes
CL=( 0 0 0 0 0 0 140 140 140 140 140 140 140 140 140 140) # Communication latency
SL=( 0 0 0 0 0 0 40 40 40 40 40 40 40 40 40 40 ) # Sort latency
BS=( 0 5 5 5 5 5 5 5 0 5 5 5 5 5 5 5 ) # Block size to transfer between queues

# Pairs of high/low water marks
# HWS=( -1 40 40 30 20 10 )
# LWS=( -1 30 30 20 10 0 ) # Based on reasonable PIFO sizes
# CL=( 0 0 140 140 140 140 ) # Communication latency
# SL=( 0 0 40 40 40 40 ) # Sort latency
# BS=( 0 0 5 5 5 5 ) # Block size to transfer between queues

# COMPS=10000
COMPS=100000
CYCLES=1000000000

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
