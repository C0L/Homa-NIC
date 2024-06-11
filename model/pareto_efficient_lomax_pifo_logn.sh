#!/bin/bash

WORKLOADS=( w1 w2 w3 w4 w5 )
ARRIVAL=( lomax )

UTILS=( .8 )
      
COMPS=1000000
CYCLES=100000000

DISTDIR=pareto_efficient_lomax_pifo_logn_dists
TRACEDIR=pareto_efficient_lomax_pifo_logn_traces

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

	# for j in $(seq 0 500 20000); do
	# for j in $(seq 0 10 2000); do
	for j in $(seq 0 100 2000); do
	    sl=$j
	    bs=8
	    cl=0

	    hw=-1
	    lw=-1

	    ./sim_pifo_logn --queue-type SRPT                                                    \
	    		--length-file ${DISTDIR}/${wk}_lengths                               \
	    		--arrival-file ${DISTDIR}/${wk}_${util}_${arrival}_arrivals          \
	    		--comps ${COMPS}                                                     \
	    		--trace-file ${TRACEDIR}/${wk}_${util}_${hw}_${lw}_${bs}_${cl}_${sl} \
	    		--high-water ${hw}                                                   \
	    		--low-water  ${lw}                                                   \
	    		--chain-latency ${cl}                                                \
	    		--block-size ${bs}                                                   \
	    		--sort-latency ${sl} &

	    # for i in {0..64}; do
	    for i in {0..1024}; do
		# hw=$(( $i*128 + 32 ))
		# lw=$(( $i*128 - 16 + 32))

		hw=$(( $i*64 + 32 ))
		lw=$(( $i*64 - 16 + 32))


	    	./sim_pifo_logn --queue-type SRPT                                                \
	    		--length-file ${DISTDIR}/${wk}_lengths                                    \
	    		--arrival-file ${DISTDIR}/${wk}_${util}_${arrival}_arrivals      \
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
