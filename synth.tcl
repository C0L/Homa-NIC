open_project -reset homa
set_top homa
add_files ./src/homa.cc
add_files -tb ./tb/homa_test.cc
open_solution "solution" -flow_target vitis
set_part {xcvu9p-flgb2104-2-i}
create_clock -period 5 -name default
#source "./dct/solution1/directives.tcl"
csim_design
#csynth_design
#cosim_design
#export_design -format ip_catalog
