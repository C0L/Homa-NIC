#!/bin/bash

WORKLOADS=( w1 )
# WORKLOADS=( w1 w2 w3 w4 w5 )
ARRIVAL=( lomax )

UTILS=( 1.05 1.1 1.2 1.3 )

COMPS=100000
CYCLES=10000000

DISTDIR=paretoefficient_lomax_pifo_logn_dists
TRACEDIR=paretoefficient_lomax_pifo_logn_traces

mkdir $DISTDIR  | :
mkdir $TRACEDIR | :

for wk in "${WORKLOADS[@]}"; do
    if [ ! -e "${DISTDIR}/${wk}_lengths" ]; then
	./distgen $wk ${CYCLES} ${DISTDIR}/${wk}_lengths
    fi

    for id in "${!UTILS[@]}"; do
	burst="${BURST[id]}"
	util="${UTILS[id]}"

	arrival=lomax

	if [ ! -e "${DISTDIR}/${wk}_${util}_${arrival}_arrivals" ]; then
	    python3.10 GenerateArrivals.py                             \
		       -d ${arrival}                                   \
		       -u $util                                        \
		       -w ${DISTDIR}/${wk}_lengths                     \
		       -a ${DISTDIR}/${wk}_${util}_${arrival}_arrivals \
		       -s ${CYCLES}
	fi

	for j in $(seq 0 100 1000); do
	    sl=$j
	    bs=16
	    cl=0

	    hw=-1
	    lw=-1

	    ./simulator --queue-type SRPT                                                    \
	    		--length-file ${DISTDIR}/${wk}_lengths                               \
	    		--arrival-file ${DISTDIR}/${wk}_${util}_${arrival}_arrivals          \
	    		--comps ${COMPS}                                                     \
	    		--trace-file ${TRACEDIR}/${wk}_${util}_${hw}_${lw}_${bs}_${cl}_${sl} \
	    		--high-water ${hw}                                                   \
	    		--low-water  ${lw}                                                   \
	    		--chain-latency ${cl}                                                \
	    		--block-size ${bs}                                                   \
	    		--sort-latency ${sl} &

	    ./simulator --queue-type SRPT                                                    \
	    		--length-file dists/${wk}_lengths                                    \
	    		--arrival-file dists/${wk}_${util}_${arrival}_arrivals      \
	    		--comps ${COMPS}                                                     \
	    		--trace-file ${TRACEDIR}/${wk}_${util}_${hw}_${lw}_${bs}_${cl}_${sl} \
	    		--high-water ${hw}                                                   \
	    		--low-water  ${lw}                                                   \
	    		--chain-latency ${cl}                                                \
	    		--block-size ${bs}                                                   \
	    		--sort-latency ${sl} &

	    for i in {10..2048}; do
		hw=$(( $i*4))
		lw=$(( $i*4 - 32 ))
	    # for i in {1..256}; do
	    # 	hw=$(( $i*8 ))
	    # 	lw=$(( $i*8 - 32 ))

	    	./simulator --queue-type SRPT                                                \
	    		--length-file dists/${wk}_lengths                                    \
	    		--arrival-file dists/${wk}_${util}_${arrival}_arrivals      \
	    		--comps ${COMPS}                                                     \
	    		--trace-file ${TRACEDIR}/${wk}_${util}_${hw}_${lw}_${bs}_${cl}_${sl} \
	    		--high-water ${hw}                                                   \
	    		--low-water  ${lw}                                                   \
	    		--chain-latency ${cl}                                                \
	    		--block-size ${bs}                                                   \
	    		--sort-latency ${sl} &

		if (( $i % 32 == 0 )) ; then
		    wait
		fi
	    done
	done
    done
done
