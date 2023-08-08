set target homa
set new_ip_paths ./ip

create_project $target $target -force

set_property is_enabled true [get_files xdc/homa.xdc]

set cur_ip_paths [get_property ip_repo_paths [current_project] ]

set_property ip_repo_paths [lappend new_ip_paths $cur_ip_paths] [current_project]

update_ip_catalog

source ./tcl/homa.tcl

create_root_design ""
save_bd_design

update_compile_order -fileset sources_1
update_compile_order -fileset sim_1

# set_property verilog_define "CL_NAME=cl_top TEST_NAME=$rtlsim_module" [get_filesets sim_1]
