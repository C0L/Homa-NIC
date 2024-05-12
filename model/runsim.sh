#!/bin/bash

# Chain latency is based on 1 way PCIe latency (check this value)
# Block size is based on what it takes to stream data onto the NIC if it takes pcie RTT to get data to the core (if 64 bytes per cycle) -- but this depends on the distribution!!!!! If every message were 1 unit, the BS should be 140
# Set the LWM to be block size
# Set the HWM to be 2 * blocksize

# Plot: instantanious queue utilization increasing on bottom artificially past 1, block size on the left. Points are difference between ideal queue and split queue mct
# Iterate through different LWM, compute HWM as LWM + 2*BS

# Color is queue structure (LWM, HWM)
# X is util
# Y is MCT

WORKLOADS=( w1 )
# WORKLOADS=( w1 w2 w3 w4 w5 )
ARRIVAL=( poisson )
UTILS=( .999 .9999 .99999 )
# UTILS=( .1 .2 .3 .4 .5 .6 .7 .8 .9 .99 .999 .9999 .99999 )

# Pairs of high/low water marks
HWS=( 60000000000 )
LWS=( 20000000000 ) # Based on reasonable PIFO sizes
# LWS=( 2 4 8 16 32 64 128 256 512 1024 2048 ) # Based on reasonable PIFO sizes

CL=( 140 ) # Based off PCIe measurements
BS=( 2 ) # Block size to transfer between queues
# BS=( 2 4 8 16 32 64 128 256 512 ) # Block size to transfer between queues
# 4294967296

CYCLES=1000000000
LATENCY=40

for wk in "${WORKLOADS[@]}"; do
    if [ ! -e "dists/${wk}_lengths" ]; then
	./distgen $wk ${CYCLES} dists/${wk}_lengths
    fi

    for util in "${UTILS[@]}"; do
	for arrival in "${ARRIVAL[@]}"; do
	    if [ ! -e "dists/${wk}_${util}_${arrival}_arrivals" ]; then
    		echo $util
		python3 GenerateArrivals.py \
			-d ${arrival} \
			-u $util \
			-w dists/${wk}_lengths \
			-a dists/${wk}_${util}_${arrival}_arrivals \
			-s ${CYCLES}
	    fi
	    for wm in "${!HWS[@]}"; do
		hw="${HWS[wm]}"
		lw="${LWS[wm]}"
		echo "HIGH AND LOW WATER"
		echo ${hw}
		echo ${lw}
		for cl in "${CL[@]}"; do
		    for bs in "${BS[@]}"; do
    			./simulator --queue-type SRPT                            \
				    --length-file dists/${wk}_lengths            \
				    --arrival-file dists/${wk}_${util}_${arrival}_arrivals  \
				    --cycles ${CYCLES}                           \
				    --trace-file traces/${wk}_${util}_${hw}_${lw}_${cl}_${bs}_${arrival}_srpt.trace \
				    --high-water ${hw}                           \
				    --low-water  ${lw}                           \
				    --chain-latency ${cl}                        \
				    --block-size ${bs}                           \
				    --sort-latency ${LATENCY}
			exit
		    done
		done
	    done
	done
    done
done

# ./simulator -q FIFO -l dists/${wk}_lengths -a dists/${wk}_${util}_arrivals -t traces/${wk}_${util}_fifo.trace -c ${CYCLES}
# ./simulator -q SRPT -l dists/${wk}_lengths -a dists/${wk}_${util}_arrivals -t traces/${wk}_${util}_srpt.trace -c ${CYCLES}
