#!/bin/bash

WORKLOADS=( w1 w2 w3 )
UTILS=( .9 .9999 )
PRIORITIES=( 8 16 )

TRACEDIR=prof_underflow

for wk in "${WORKLOADS[@]}"; do
    for util in "${UTILS[@]}"; do
	for priority in "${PRIORITIES[@]}"; do
	    python3 plot_pifo_occupancy.py -t  ${TRACEDIR}/${wk}_${util}_${priority}.stats -f img/pifo_underflow_${wk}_${util}_${priority}.png
	done
    done
done
