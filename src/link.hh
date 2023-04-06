#include "homa.hh"

void proc_link_ingress(hls::stream<raw_frame_t> & link_ingress, char * ddr_ram, user_output_t * dma_egress);
void proc_link_egress(hls::stream<raw_frame_t> & link_egress);

