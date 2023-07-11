#ifndef DATABUFF_H
#define DATABUFF_H

#include "homa.hh"
extern "C"{
void dbuff_stack(hls::stream<sendmsg_t, VERIF_DEPTH> & sendmsg_i,
      hls::stream<sendmsg_t, VERIF_DEPTH> & sendmsg_o,
      hls::stream<hls::axis<dma_r_req_t,0,0,0>> & dma_read_o);
}
extern "C"{
void dbuff_ingress(hls::stream<in_chunk_t, VERIF_DEPTH> & chunk_in_o,
      hls::stream<hls::axis<dma_w_req_t,0,0,0>> & dma_w_req_o,
      hls::stream<header_t, VERIF_DEPTH> & header_in_i);
}
extern "C"{
void dbuff_egress(hls::stream<hls::axis<dbuff_in_t,0,0,0>> & dbuff_egress_i,
      hls::stream<dbuff_notif_t, VERIF_DEPTH> & dbuff_notif_o,
      hls::stream<out_chunk_t, VERIF_DEPTH> & out_chunk_i,
      hls::stream<raw_stream_t> & link_egress);
} 
#endif
