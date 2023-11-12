# set argc [llength $argv]
# if { $argc < 6 } {
#     set errmsg "Usage: <part> <job type> <c src> <json src> <homa cfg>"
#     puts $errmsg
#     return 1
# }

set part [lindex $argv 0] 
set job_type [lindex $argv 1] 
set c_src [lindex $argv 2] 
set json_src [lindex $argv 3] 
set test_bench [lindex $argv 4]
set top [lindex $argv 5]

puts $part
puts $job_type
puts $c_src
puts $json_src
puts $test_bench
puts $top

set homa_cfg ""

foreach cfg [lrange $argv 6 end] {
    set homa_cfg "$homa_cfg -D$cfg"
}

puts $homa_cfg

open_project -reset $top\_kernel
set_top $top 

open_solution -reset "solution" -flow_target vivado 
set_part $part
create_clock -period 5 -name default

# This is the neccesary pipeline style for tasks
config_compile -pipeline_style flp

if {$job_type == 0} {
    # Csim only
    add_files $c_src -cflags "-DCSIM $homa_cfg"
    add_files -tb $test_bench -cflags "-DCSIM $homa_cfg"
    csim_design -setup
} elseif {$job_type == 1} {
    add_files $c_src -cflags "-DSYNTH $homa_cfg"
    if {$json_src != "null"} {
	add_files -blackbox $json_src
    }
    csynth_design
    export_design -output ip/$top/ -format ip_catalog
} elseif {$job_type == 2} {
    add_files $c_src -cflags "-DCOSIM $homa_cfg"
    add_files -tb $test_bench -cflags "-DCOSIM $homa_cfg"
    # config_dataflow -fifo_depth 26 -start_fifo_depth 26 -scalar_fifo_depth 26 -task_level_fifo_depth 26
    csynth_design
    cosim_design -trace_level all -wave_debug -mflags "-l 64"
}

exit
