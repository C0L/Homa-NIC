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

open_project -reset homa_kern
set_top homa

add_files $c_src

if {$job_type == 0 || $job_type == 2 } {
    add_files -tb $test_bench
}

add_files -blackbox $json_src

open_solution -reset "solution" -flow_target vivado 
set_part $part
create_clock -period 4 -name default

# This is the neccesary pipeline style for tasks
config_compile -pipeline_style flp

if {$job_type == 0} {
    # Csim only
    csim_design
} elseif {$job_type == 1} {
    # Csynth only
    csynth_design
    export_design -output ip/ -format ip_catalog
} elseif {$job_type == 2} {
    # config_dataflow -fifo_depth 512 -start_fifo_depth 512 -scalar_fifo_depth 512 -task_level_fifo_depth 512 -override_user_fifo_depth 512


    #config_dataflow -start_fifo_depth 512 -scalar_fifo_depth 512 -task_level_fifo_depth 512 -override_user_fifo_depth 512 fails

    # config_dataflow -start_fifo_depth 512 -scalar_fifo_depth 512 -task_level_fifo_depth 512 -override_user_fifo_depth 512 
    
    # config_dataflow -scalar_fifo_depth 512 -task_level_fifo_depth 512 -override_user_fifo_depth 512
    # config_dataflow -start_fifo_depth 512 -scalar_fifo_depth 512 -override_user_fifo_depth 512 fails
    # config_dataflow -start_fifo_depth 512 -scalar_fifo_depth 512 -task_level_fifo_depth 512 fails

    # config_dataflow -override_user_fifo_depth 512
    csynth_design
    cosim_design 
}

exit
