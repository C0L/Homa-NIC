#!/bin/bash

WORKLOADS=( w1 )
ARRIVAL=( poisson )
# UTILS=(     1   1.5     2      3     4     6     8    12    16    24    32    52    64    92   128   200   256   350   512   812  1024  1600  2048  3000  4096 )
# BURST=( 16384 16384 16384  16384 16384 16384 16384 16384 16384 16384 16384 16384 16384 16384 16384 16384 16384 16384 16384 16384 16384 16384 16384 16384 16384 )

UTILS=( 100   200 )
BURST=( 16384 16384 )

# UTILS=( 1     2    4    8    16   32  64  128 256 512 1024 2048 4096 8192 16384 )
# BURST=( 16384 8192 4096 2048 1024 512 256 128 64  32  16   8    4    2   1 )

WARM=1000000
COMPS=2000000
CYCLES=3000000

# TODO need to make stats gathering only during burst?


HWS=( -1 128 1024 2048 4096 5000 6000 7000 8192 9000 10000 11000 12000 13000 14000 15000 16384 )
LWS=( -1 118 1014 2038 4086 4490 5990 6990 8182 8990 9990  10990 11990 12990 13990 14990 16374 )
CL=(   0   0   0    0     0    0    0    0    0    0    0    0    0        0     0     0     0   ) 
SL=(   0   0   0    0     0    0    0    0    0    0    0    0    0        0     0     0     0   ) 
BS=(   0   5   5    5     5    5    5    5    5    5    5    5    5        5     5     5     5  )


# HWS=( -1 20 40 60 80 100 120 140 160 180 200 220 240 260 280 300 320 340 360 380 400 420 440 460 480 500 520 540 560 580 600 620 640 660 680 700 )
# LWS=( -1 10 30 50 70 90  110 130 150 170 190 210 230 250 270 290 310 330 350 370 390 410 430 450 470 490 510 530 550 570 590 610 630 650 670 690 )
# CL=(   0  0  0  0  0  0    0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0 ) # Communication latency
# SL=(   0  0  0  0  0  0    0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0 ) # Sort latency
# BS=(   0  5  5  5  5  5    5   5   5   5   5   5   5   5   5   5   5   5   5   5   5   5   5   5   5   5   5   5   5   5   5   5   5   5   5   5 ) # Block size to transfer between queues

for wk in "${WORKLOADS[@]}"; do
    if [ ! -e "dists/${wk}_lengths" ]; then
	./distgen $wk ${CYCLES} dists/${wk}_lengths
    fi

    for id in "${!UTILS[@]}"; do
	burst="${BURST[id]}"
	util="${UTILS[id]}"
	# for burst in "${BURST[@]}"; do

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

	for wm in "${!HWS[@]}"; do
	    hw="${HWS[wm]}"
	    lw="${LWS[wm]}"
	    cl="${CL[wm]}"
	    sl="${SL[wm]}"
	    bs="${BS[wm]}"

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
