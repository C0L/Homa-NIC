set argc [llength $argv]
if { $argc != 5 } {
    set errmsg "Usage: <part> <job type> <c src> <json src>"
    puts $errmsg
    return 1
}

set part [lindex $argv 0] 
set job_type [lindex $argv 1] 
set c_src [lindex $argv 2] 
set json_src [lindex $argv 3] 
set test_bench [lindex $argv 4] 

open_project -reset homa
set_top homa

#add_files ./src/rpcmgmt.cc
#add_files ./src/srptmgmt.cc
#add_files ./src/homa.cc
#add_files ./src/dma.cc
#add_files ./src/link.cc
#add_files ./src/peer.cc
#add_files ./src/databuff.cc
#add_files ./src/timer.cc
add_files $c_src

if {$job_type == 0} {
   add_files -tb $test_bench
}

add_files -blackbox $json_src

# add_files -blackbox ./src/srpt_grant_pkts.json

open_solution -reset "solution" -flow_target vivado 
set_part $part
#set_part {xcvu9p-flgb2104-2-i}
create_clock -period 3.1 -name default

if {$job_type == 0} {
   # Csim only
   csim_design

} elseif {$job_type == 1} {
	# Csynth only
	csynth_design
	
} 

#elseif {$hls_exec == 2} {
#	# Run Synthesis, RTL Simulation and Exit
#	csynth_design
#	
#	cosim_design
#} elseif {$hls_exec == 3} { 
#	# Run Synthesis, RTL Simulation, RTL implementation and Exit
#	csynth_design
#	
#	cosim_design
#	export_design
#} else {
#	# Default is to exit after setup
#	csynth_design
#}

exit
