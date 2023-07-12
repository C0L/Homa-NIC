#ifndef DATABUFF_H
#define DATABUFF_H

#include "homa.hh"
extern "C"{
void dbuff_stack(hls::stream<sendmsg_t, VERIF_DEPTH> & sendmsg_i,
      hls::stream<sendmsg_t, VERIF_DEPTH> & sendmsg_o,
      hls::stream<dma_r_req_t, VERIF_DEPTH> & dma_read_o);
}
extern "C"{
void dbuff_ingress(hls::stream<in_chunk_t, VERIF_DEPTH> & chunk_in_o,
      hls::stream<dma_w_req_t, VERIF_DEPTH> & dma_w_req_o,
      hls::stream<header_t, VERIF_DEPTH> & header_in_i);
}
extern "C"{
void dbuff_egress(hls::stream<dbuff_in_t, VERIF_DEPTH> & dbuff_egress_i,
      hls::stream<dbuff_notif_t, VERIF_DEPTH> & dbuff_notif_o,
      hls::stream<out_chunk_t, VERIF_DEPTH> & out_chunk_i,
      hls::stream<raw_stream_t> & link_egress);
} 
#endif
