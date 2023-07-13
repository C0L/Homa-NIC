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
create_clock -period 20 -name default
# create_clock -period 3.1 -name default

# config_dataflow -default_channel fifo -disable_fifo_sizing_opt -override_user_fifo_depth 128 -fifo_depth 2
# config_dataflow -default_channel fifo -fifo_depth 128 -disable_fifo_sizing_opt -override_user_fifo_depth 128 -start_fifo_depth 128 -scalar_fifo_depth 128 -task_level_fifo_depth 128 
# config_dataflow -fifo_depth 256 -start_fifo_depth 256 -scalar_fifo_depth 256 -task_level_fifo_depth 256
# config_dataflow -default_channel fifo -fifo_depth 2

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
	cosim_design 
   #-disable_deadlock_detection
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
