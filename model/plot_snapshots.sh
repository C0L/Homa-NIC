#!/bin/bash

WORKLOADS=( w4 )
UTILS=( .9999 )
PRIORITIES=( 16 )

TRACEDIR=prof_snapshots

for wk in "${WORKLOADS[@]}"; do
    for util in "${UTILS[@]}"; do
	for priority in "${PRIORITIES[@]}"; do
	    python3 plot_snapshots_cdf.py -t  ${TRACEDIR}/${wk}_${util}_${priority}_* -f img/snapshots_cdf_${wk}_${util}_${priority}.png
	done
    done
done
