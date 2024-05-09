#!/bin/bash

WORKLOADS=( w1 w2 w3 w4 w5 )
UTILS=( .1 )
HWS=( 40 )
LWS=( 20 )
CL=( 20 )
BS=( 10 )
# UTILS=( .9 .7 .5 .3 .1 )
CYCLES=10000000
# CYCLES=1000000
# CYCLES=10000000

for wk in "${WORKLOADS[@]}"; do
    ./distgen $wk ${CYCLES} dists/${wk}_lengths

    for util in "${UTILS[@]}"; do
    	echo $util
	python3 GenerateArrivals.py -d pareto -u $util -w dists/${wk}_lengths -a dists/${wk}_${util}_arrivals -s ${CYCLES}
	for hw in "${HWS[@]}"; do
	    for lw in "${LWS[@]}"; do
		for cl in "${CL[@]}"; do
		    for bs in "${BS[@]}"; do
    			./simulator --queue-type SRPT                            \
				    --length-file dists/${wk}_lengths            \
				    --arrival-file dists/${wk}_${util}_arrivals  \
				    --cycles ${CYCLES}                           \
				    --trace-file traces/${wk}_${util}_${hw}_${lw}_${cl}_${bs}_srpt.trace \
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
