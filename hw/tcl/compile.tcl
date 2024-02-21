# set target homa
# set bd homa
set part xcu250-figd2104-2L-e
set board xilinx.com:au250:part0:1.3 

create_project homa homa -force -part $part

set_property board_part xilinx.com:au250:part0:1.3 [current_project]

# Constraints
add_files -fileset constrs_1 ./xdc/homa.xdc
set_property is_enabled true [get_files ./xdc/homa.xdc]

# add_files -fileset constrs_1 ./xdc/alveo-u250-xdc.xdc
# set_property is_enabled true [get_files ./xdc/alveo-u250-xdc.xdc]

# Source files 
add_files -fileset sources_1 ./src/main/resouces
add_files -fileset sources_1 ./gen

set_property top top [current_fileset]

update_compile_order -fileset sources_1
update_compile_order -fileset sim_1

foreach script [glob ./gen/*.tcl] {source $script}
source ./tcl/xilinx_ip.tcl

# Launch Synthesis
launch_runs synth_1
wait_on_run synth_1
open_run synth_1 -name netlist_1

# Generate a timing and power reports and write to disk
# report_timing_summary -delay_type max -report_unconstrained -check_timing_verbose -max_paths 10 -input_pins -file ...

# Launch Implementation
launch_runs impl_1 -to_step write_bitstream
wait_on_run impl_1

# Generate a timing and power reports and write to disk
open_run impl_1

# report_timing_summary -delay_type min_max -report_unconstrained -check_timing_verbose -max_paths 10 -input_pins -file ./Tutorial_Created_Data/project_bft/imp_timing.rpt report_power -file ./Tutorial_Created_Data/project_bft/imp_power.rpt
