#include "homa.hh"

void proc_link_ingress(hls::stream<raw_frame_t> & link_ingress, homa_rpc_t * rpcs, char * ddr_ram, rpc_stack_t & rpc_stack, user_output_t * dma_egress);
void proc_link_egress(hls::stream<raw_frame_t> & link_egress, homa_rpc_t * rpcs, rpc_stack_t & rpc_stack, srpt_queue_t & srpt_queue);

