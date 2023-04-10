open_project -reset homa
set_top homa
add_files ./src/homa.hh
add_files ./src/rpcmgmt.hh
add_files ./src/rpcmgmt.cc
add_files ./src/srptmgmt.hh
add_files ./src/srptmgmt.cc
add_files ./src/homa.hh
add_files ./src/homa.cc
add_files ./src/dma.cc
add_files ./src/dma.hh
add_files ./src/link.hh
add_files ./src/link.cc
add_files ./src/net.hh
add_files ./src/hash.hh
add_files ./src/peer.hh
add_files ./src/peer.cc
set_param general.maxThreads 16
open_solution -reset "solution" -flow_target vivado 
set_part {xcvu9p-flgb2104-2-i}
create_clock -period 5 -name default
csynth_design
#cosim_design
#export_design -format ip_catalog
