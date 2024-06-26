#!/bin/bash

WORKLOADS=( w1 w2 w3 w4 w5 )
ARRIVAL=( lomax )

UTILS=( .7 .8 .9 )
      
# COMPS=1000000000
# CYCLES=1100000000

COMPS=1000000
CYCLES=1100000

DISTDIR=lomax_prof_dists
TRACEDIR=lomax_prof_traces

mkdir $DISTDIR  | :
mkdir $TRACEDIR | :

for wk in "${WORKLOADS[@]}"; do
    {
	if [ ! -e "${DISTDIR}/${wk}_lengths" ]; then
	    ./distgen $wk ${CYCLES} ${DISTDIR}/${wk}_lengths
	fi

	for util in "${UTILS[@]}"; do
	    arrival=lomax
	    if [ ! -e "${DISTDIR}/${wk}_${util}_${arrival}_arrivals" ]; then
		python3 GenerateArrivals.py                                \
	    		   -d ${arrival}                                   \
	    		   -u $util                                        \
	    		   -w ${DISTDIR}/${wk}_lengths                     \
	    		   -a ${DISTDIR}/${wk}_${util}_${arrival}_arrivals \
	    		   -s ${CYCLES}
	    fi

	    ./sim_ideal --length-file ${DISTDIR}/${wk}_lengths                       \
	    		--arrival-file ${DISTDIR}/${wk}_${util}_${arrival}_arrivals  \
	    		--comps ${COMPS}                                             \
	    		--trace-file ${TRACEDIR}/${wk}_${util} &
	done
	wait
    } &
done

wait
