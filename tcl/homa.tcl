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

add_files $c_src

if {$job_type == 0 || $job_type == 2 } {
   add_files -tb $test_bench
}

add_files -blackbox $json_src

open_solution -reset "solution" -flow_target vivado 
set_part $part
create_clock -period 3.1 -name default

# This is the neccesary pipeline style for tasks
config_compile -pipeline_style flp

if {$job_type == 0} {
   # Csim only
   csim_design

} elseif {$job_type == 1} {
   # Csynth only
   csynth_design
	
} elseif {$job_type == 2} {
	# Run Synthesis, RTL Simulation and Exit
	csynth_design
   # cosim_stall -generate cosim_stall.json
   # cosim_stall -check cosim_stall.json
   # Enable FIFO sizing seems to prevent the tool from spamming sendmsg/recvmsg requests at the interface
	cosim_design 
# -user_stall cosim_stall.json
} 
# elseif {$hls_exec == 3} { 
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
