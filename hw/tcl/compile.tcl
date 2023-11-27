set target homa
set bd homa
set new_ip_paths [list "./ip/homa" "./ip/interface"]
set part xcu250-figd2104-2L-e
set board xilinx.com:au250:part0:1.3 

create_project $target $target -force -part $part

set_property board_part xilinx.com:au250:part0:1.3 [current_project]

add_files -fileset constrs_1 ./xdc/homa.xdc
add_files -fileset sources_1 ./ip/picorv32/picorv32.v
set_property is_enabled true [get_files ./xdc/homa.xdc]
add_files -fileset constrs_1 ./xdc/alveo-u250-xdc.xdc
set_property is_enabled true [get_files ./xdc/alveo-u250-xdc.xdc]

set cur_ip_paths [get_property ip_repo_paths [current_project]]
set_property ip_repo_paths [concat $new_ip_paths $cur_ip_paths] [current_project]
# set_property ip_repo_paths [lappend new_ip_paths $cur_ip_paths] [current_project]
update_ip_catalog

source ./tcl/homa.tcl

make_wrapper -files [get_files ./$target/$target.srcs/sources_1/bd/$bd/$bd.bd] -top
add_files -norecurse ./$target/$target.srcs/sources_1/bd/$bd/hdl/$bd\_wrapper.v

set_property top $target\_wrapper [current_fileset]

update_compile_order -fileset sources_1
update_compile_order -fileset sim_1

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
