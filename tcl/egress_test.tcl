open_project -reset homa_egress_test
set_top homa
add_files ./src/rpcmgmt.cc 
add_files ./src/rpcmgmt.hh 
add_files ./src/srptmgmt.cc
add_files ./src/srptmgmt.hh
add_files ./src/homa.cc 
add_files ./src/homa.hh
add_files ./src/dma.cc
add_files ./src/dma.hh
add_files ./src/link.cc
add_files ./src/link.hh 
add_files ./src/xmitbuff.cc
add_files ./src/xmitbuff.hh
add_files ./src/timer.cc
add_files ./src/timer.hh
add_files ./src/net.hh
add_files ./src/peer.hh 
add_files ./src/peer.cc
add_files ./src/stack.hh
add_files -tb ./tb/egress_test.cc
open_solution -reset "solution" -flow_target vitis
set_part {xcvu9p-flgb2104-2-i}
create_clock -period 5 -name default
csim_design  