WORKLOADS=( w1 w2 w3 )

UTILS=( .9999 )

COMPS=100000000
# COMPS=100000000

TRACEDIR=qperf_traces

mkdir $TRACEDIR | :

for wk in "${WORKLOADS[@]}"; do
    for util in "${UTILS[@]}"; do
     	# ./sim_pifo_naive --workload ${wk} \
     	#  	         --priorities 8  \
     	#  	         --utilization ${util} \
     	#  	         --comps ${COMPS}      \
     	#      	         --trace-file /dev/null &> ${TRACEDIR}/${wk}_${util}_pifo_naive_8.log &


     	./sim_sp_naive --workload ${wk} \
     		         --priorities 16  \
     		         --utilization ${util} \
     		         --comps ${COMPS}      \
			 --trace-file /dev/null &> ${TRACEDIR}/${wk}_${util}_sp_naive_16.log &

     	./sim_pifo_naive --workload ${wk} \
     		         --priorities 16  \
     		         --utilization ${util} \
     		         --comps ${COMPS}      \
			 --trace-file /dev/null &> ${TRACEDIR}/${wk}_${util}_pifo_naive_16.log &

	./sim_strict_priorities --workload ${wk} \
	 			--priorities 8  \
	 			--utilization ${util} \
	 			--comps ${COMPS}      \
	 			--trace-file /dev/null &> ${TRACEDIR}/${wk}_${util}_strict_priorities_8.log &

     	./sim_strict_priorities --workload ${wk} \
     		                --priorities 16  \
     		                --utilization ${util} \
     		                --comps ${COMPS}      \
     	    	                --trace-file /dev/null &> ${TRACEDIR}/${wk}_${util}_strict_priorities_16.log &

     	./sim_strict_priorities --workload ${wk} \
     		                --priorities 32  \
     		                --utilization ${util} \
     		                --comps ${COMPS}      \
     	    	                --trace-file /dev/null &> ${TRACEDIR}/${wk}_${util}_strict_priorities_32.log &

     	./sim_strict_priorities --workload ${wk} \
     		                --priorities 64  \
     		                --utilization ${util} \
     		                --comps ${COMPS}      \
     	    	                --trace-file /dev/null &> ${TRACEDIR}/${wk}_${util}_strict_priorities_64.log &

    	./sim_fifo_naive --workload ${wk} \
     		         --priorities 8 \
     		         --utilization ${util} \
     		         --comps ${COMPS}      \
     	    	         --trace-file /dev/null &> ${TRACEDIR}/${wk}_${util}_fifo_naive_8.log &

     	./sim_fifo_naive --workload ${wk} \
     		         --priorities 16 \
     		         --utilization ${util} \
     		         --comps ${COMPS}      \
     	    	         --trace-file /dev/null &> ${TRACEDIR}/${wk}_${util}_fifo_naive_16.log &

     	#  ./sim_ideal --workload ${wk}     \
     	#  	    --utilization ${util} \
     	#  	    --comps ${COMPS}      \
     	#      	    --trace-file /dev/null &> ${TRACEDIR}/${wk}_${util}_ideal.log &
    done
done
