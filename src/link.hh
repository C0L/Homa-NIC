void proc_link_egress(hls::stream<raw_frame> & link_egress, homa_rpc (&rpcs)[MAX_RPCS], rpc_queue & send_ready);
void proc_link_ingress(hls::stream<raw_frame> & link_ingress, homa_rpc (&rpcs)[MAX_RPCS], dma_egress);

