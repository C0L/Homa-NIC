#!/bin/bash

WORKLOADS=( w1 w2 w3 w4 w5 )
UTILS=( .9 .7 .5 .3 .1 )
CYCLES=1000000

for wk in "${WORKLOADS[@]}";
do
    ./distgen $wk ${CYCLES} > dists/${wk}_lengths

    for util in "${UTILS[@]}"; do
	echo $util
	python3 GenerateArrivals.py -d pareto -u $util -w dists/${wk}_lengths -a dists/${wk}_${util}_arrivals -s ${CYCLES}
	./simulator "FIFO" dists/${wk}_lengths dists/${wk}_${util}_arrivals traces/${wk}_${util}_fifo.trace ${CYCLES}
	./simulator "SRPT" dists/${wk}_lengths dists/${wk}_${util}_arrivals traces/${wk}_${util}_srpt.trace ${CYCLES}
    done

    python3 Plot.py -t traces/${wk}_*_fifo.trace -f mm1_${wk}_fifo.png
    python3 Plot.py -t traces/${wk}_*_srpt.trace -f mm1_${wk}_srpt.png 
done

