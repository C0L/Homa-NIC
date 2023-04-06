open_project -reset homa
set_top homa
add_files -tb ./tb/homa_test.cc
add_files -tb ./src/rpcmgmt.hh
add_files -tb ./src/rpcmgmt.cc
add_files -tb ./src/homa.hh
add_files -tb ./src/homa.cc
add_files -tb ./src/dma.cc
add_files -tb ./src/dma.hh
add_files -tb ./src/link.hh
add_files -tb ./src/link.cc
open_solution -reset "solution" -flow_target vitis
set_part {xcvu9p-flgb2104-2-i}
create_clock -period 5 -name default
#source "./dct/solution1/directives.tcl"
csim_design
