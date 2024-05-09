#!/bin/bash

WORKLOADS=( w1 w2 w3 w4 w5 )
UTILS=( .9 .7 .5 .3 .1 )
CYCLES=10000
# CYCLES=1000000
# CYCLES=10000000

for wk in "${WORKLOADS[@]}";
do
    ./distgen $wk ${CYCLES} dists/${wk}_lengths

    for util in "${UTILS[@]}"; do
    	echo $util
    	python3 GenerateArrivals.py -d pareto -u $util -w dists/${wk}_lengths -a dists/${wk}_${util}_arrivals -s ${CYCLES}
    	./simulator --queue-type SRPT                            \
		    --length-file dists/${wk}_lengths            \
		    --arrival-file dists/${wk}_${util}_arrivals  \
	            --cycles ${CYCLES}                           \
		    --trace-file traces/${wk}_${util}_fifo.trace \
		    --high-water 200                             \
		    --low-water  100                             \
		    --chain-latency 20                           \
		    --block-size 10
    	# ./simulator -q FIFO -l dists/${wk}_lengths -a dists/${wk}_${util}_arrivals -t traces/${wk}_${util}_fifo.trace -c ${CYCLES}
    	# ./simulator -q SRPT -l dists/${wk}_lengths -a dists/${wk}_${util}_arrivals -t traces/${wk}_${util}_srpt.trace -c ${CYCLES}
    done
done
