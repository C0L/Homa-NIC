#ifndef DATABUFF_H
#define DATABUFF_H

#include "homa.hh"
extern "C"{
   void dbuff_stack(hls::stream<sendmsg_t> & sendmsg_i,
         hls::stream<sendmsg_t> & sendmsg_o,
         hls::stream<dma_r_req_t> & dma_read_o);

   void dbuff_ingress(hls::stream<in_chunk_t> & chunk_in_o,
         hls::stream<dma_w_req_t> & dma_w_req_o,
         hls::stream<header_t> & header_in_i);

   void dbuff_egress(hls::stream<dbuff_in_t> & dbuff_egress_i,
         hls::stream<dbuff_notif_t> & dbuff_notif_o,
         hls::stream<out_chunk_t> & out_chunk_i,
         hls::stream<out_chunk_t> & out_chunk_o);
} 
#endif
