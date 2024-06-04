#!/bin/bash

WORKLOADS=( w1 w2 w3 w4 w5 )
ARRIVAL=( lomax )

UTILS=( 1.05 1.1 1.2 1.3 )
BURST=( 999 999 999 999 )
#UTILS=( .9 .99 .999 .9999 .99999 )
#BURST=( 999 999 999  999    999 )
# UTILS=( 3.0 3.0 3.0 )
# BURST=( 1024 2048 4096 )

# WARM=1000000
# COMPS=2000000
# CYCLES=3000000

# COMPS=100000000
# CYCLES=200000000
      
COMPS=100000
CYCLES=10000000

for wk in "${WORKLOADS[@]}"; do
    if [ ! -e "dists/${wk}_lengths" ]; then
	./distgen $wk ${CYCLES} dists/${wk}_lengths
    fi

    for id in "${!UTILS[@]}"; do
	burst="${BURST[id]}"
	util="${UTILS[id]}"

	arrival=lomax

	if [ ! -e "dists/${wk}_${util}_${arrival}_${burst}_arrivals" ]; then
	    python3.10 GenerateArrivals.py                     \
		       -d ${arrival}                              \
		       -u $util                                   \
		       -w dists/${wk}_lengths                     \
		       -a dists/${wk}_${util}_${arrival}_${burst}_arrivals    \
		       -s ${CYCLES}
	fi

	# if [ ! -e "dists/${wk}_${util}_${arrival}_${burst}_arrivals" ]; then
    	#     echo $util
	#     python3.10 GenerateArrivals.py                   \
	#     	       -d ${arrival}                            \
	#     	       -u .9                                    \
	#     	       -w dists/${wk}_lengths                   \
	#     	       -a dists/${wk}_${util}_${arrival}_steady \
	#     	       -s ${WARM}

	#     python3.10 GenerateArrivals.py                     \
	#     	       -d ${arrival}                              \
	#     	       -u $util                                   \
	#     	       -w dists/${wk}_lengths                     \
	#     	       -a dists/${wk}_${util}_${arrival}_burst    \
	#     	       -s ${burst}

	#     cat dists/${wk}_${util}_${arrival}_steady dists/${wk}_${util}_${arrival}_burst dists/${wk}_${util}_${arrival}_steady > dists/${wk}_${util}_${arrival}_${burst}_arrivals
	# fi

	#for j in $(seq -f "%03g" 0 1 999); do
	# for j in $(seq -f "%02g" 0 .02 1); do
	for j in $(seq -f "%02g" 0 .05 1); do
	    sl=$j
	    bs=16
	    # bs=4
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

	    # for i in {12..256}; do
	    # for i in {16..512}; do
	    for i in {10..2048}; do
		hw=$(( $i*4))
		lw=$(( $i*4 - 32 ))
		# hw=$(( $i*4 ))
		# lw=$(( $i*4 - 32 ))
		#hw=$(( $i*256 ))
		#lw=$(( $i*256 - 32 ))

		# hw=$(( $i*16 ))
		# lw=$(( $i*16 - 8 ))

	    	./simulator --queue-type SRPT                            \
	    		--length-file dists/${wk}_lengths            \
	    		--arrival-file dists/${wk}_${util}_${arrival}_${burst}_arrivals  \
	    		--comps ${COMPS}                             \
	    		--trace-file traces/${wk}_${util}_${hw}_${lw}_${bs}_${cl}_${sl}_${burst}_burst \
	    		--high-water ${hw}                           \
	    		--low-water  ${lw}                           \
	    		--chain-latency ${cl}                        \
	    		--block-size ${bs}                           \
	    		--sort-latency ${sl} &

		if (( $i % 64 == 0 )) ; then
		    wait
		fi
	    done
	done
    done
done
