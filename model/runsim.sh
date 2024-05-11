#!/bin/bash

WORKLOADS=( w1 )
# WORKLOADS=( w1 w2 w3 w4 w5 )
ARRIVAL=( lomax )
UTILS=( .9 )

# Pairs of high/low water marks
HWS=( 60000000000 )
LWS=( 60000000000 )

CL=( 140 ) # Based off PCIe measurements
BS=( 8 ) # Block size to transfer between queues
CYCLES=1000000

for wk in "${WORKLOADS[@]}"; do
    ./distgen $wk ${CYCLES} dists/${wk}_lengths

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
				    --block-size ${bs}
		    done
		done
	    done
	done
    done
done

# ./simulator -q FIFO -l dists/${wk}_lengths -a dists/${wk}_${util}_arrivals -t traces/${wk}_${util}_fifo.trace -c ${CYCLES}
# ./simulator -q SRPT -l dists/${wk}_lengths -a dists/${wk}_${util}_arrivals -t traces/${wk}_${util}_srpt.trace -c ${CYCLES}
