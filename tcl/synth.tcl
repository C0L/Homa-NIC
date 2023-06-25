open_project -reset homa
set_top homa
add_files ./src/rpcmgmt.cc
add_files ./src/srptmgmt.cc
add_files ./src/homa.cc
add_files ./src/dma.cc
add_files ./src/link.cc
add_files ./src/peer.cc
add_files ./src/databuff.cc
add_files ./src/timer.cc
add_files -blackbox ./src/srpt_grant_pkts.json
open_solution -reset "solution" -flow_target vivado
set_part {xcvu9p-flgb2104-2-i}
create_clock -period 3.1 -name default
csynth_design
#cosim_design
