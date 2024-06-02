#!/bin/bash

WORKLOADS=( w1 w2 w3 w4 w5 )
ARRIVAL=( poisson )

UTILS=( 3.0 3.0 3.0 )
BURST=( 1024 2048 4096 )

WARM=1000000
COMPS=2000000
CYCLES=3000000

# TODO could vary the LWM to reduce communication, but the workload needs to relect this

for wk in "${WORKLOADS[@]}"; do
    if [ ! -e "dists/${wk}_lengths" ]; then
	./distgen $wk ${CYCLES} dists/${wk}_lengths
    fi

    for id in "${!UTILS[@]}"; do
	burst="${BURST[id]}"
	util="${UTILS[id]}"

	arrival=poisson

	if [ ! -e "dists/${wk}_${util}_${arrival}_${burst}_arrivals" ]; then
    	    echo $util
	    python3.10 GenerateArrivals.py                   \
	    	       -d ${arrival}                            \
	    	       -u .9                                    \
	    	       -w dists/${wk}_lengths                   \
	    	       -a dists/${wk}_${util}_${arrival}_steady \
	    	       -s ${WARM}

	    python3.10 GenerateArrivals.py                     \
	    	       -d ${arrival}                              \
	    	       -u $util                                   \
	    	       -w dists/${wk}_lengths                     \
	    	       -a dists/${wk}_${util}_${arrival}_burst    \
	    	       -s ${burst}

	    cat dists/${wk}_${util}_${arrival}_steady dists/${wk}_${util}_${arrival}_burst dists/${wk}_${util}_${arrival}_steady > dists/${wk}_${util}_${arrival}_${burst}_arrivals
	fi

	for j in $(seq -f "%02g" 0 2 99); do
	    sl=.$j
	    bs=4
	    cl=0

	    hw=-1
	    lw=-1

	    ./simulator --queue-type SRPT                            \
	    		--length-file dists/${wk}_lengths            \
	    		--arrival-file dists/${wk}_${util}_${arrival}_${burst}_arrivals  \
	    		--comps ${COMPS}                             \
	    		--trace-file traces/${wk}_${util}_${hw}_${lw}_${bs}_${cl}_${sl}_${burst}_burst \
	    		--high-water ${hw}                           \
	    		--low-water  ${lw}                           \
	    		--chain-latency ${cl}                        \
	    		--block-size ${bs}                           \
	    		--sort-latency ${sl}

	    ./simulator --queue-type SRPT                            \
	    		--length-file dists/${wk}_lengths            \
	    		--arrival-file dists/${wk}_${util}_${arrival}_${burst}_arrivals  \
	    		--comps ${COMPS}                             \
	    		--trace-file traces/${wk}_${util}_${hw}_${lw}_${bs}_${cl}_${sl}_${burst}_burst \
	    		--high-water ${hw}                           \
	    		--low-water  ${lw}                           \
	    		--chain-latency ${cl}                        \
	    		--block-size ${bs}                           \
	    		--sort-latency ${sl}

	    for i in {4..32}; do
	    # for i in {4..128}; do
		hw=$(( $i*64 ))
		lw=$(( $i*64 - 8 ))


		# hw=$(( $i*4 ))
		# lw=$(( $i*4 - 8 ))

	    	./simulator --queue-type SRPT                            \
	    		--length-file dists/${wk}_lengths            \
	    		--arrival-file dists/${wk}_${util}_${arrival}_${burst}_arrivals  \
	    		--comps ${COMPS}                             \
	    		--trace-file traces/${wk}_${util}_${hw}_${lw}_${bs}_${cl}_${sl}_${burst}_burst \
	    		--high-water ${hw}                           \
	    		--low-water  ${lw}                           \
	    		--chain-latency ${cl}                        \
	    		--block-size ${bs}                           \
	    		--sort-latency ${sl}
	    done
	done
    done
done
