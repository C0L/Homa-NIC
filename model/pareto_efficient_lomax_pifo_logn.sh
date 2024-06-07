#!/bin/bash

WORKLOADS=( w1 )
# WORKLOADS=( w1 w2 w3 w4 w5 )
ARRIVAL=( lomax )

UTILS=( 1.05 )
# UTILS=( 1.05 1.1 1.2 1.3 )

COMPS=1000000
CYCLES=10000000

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

	for j in $(seq 0 100 10000); do
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
	    

	    for i in {4..1024}; do
	    # 	hw=$(( $i*32))
	    # 	lw=$(( $i*32 - 32 ))
	    # for i in {..256}; do
		hw=$(( $i*8 ))
		lw=$(( $i*8 - 16 ))

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
