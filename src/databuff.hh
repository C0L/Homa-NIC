#ifndef DATABUFF_H
#define DATABUFF_H

#include "homa.hh"
extern "C"{
   void dbuff_stack(hls::stream<onboard_send_t> & onboard_send_i,
         hls::stream<onboard_send_t> & onboard_send_o,
         hls::stream<dma_r_req_raw_t> & dma_read_o);

   void dbuff_ingress(hls::stream<in_chunk_t> & chunk_in_o,
         hls::stream<dma_w_req_raw_t> & dma_w_req_o,
         hls::stream<header_t> & header_in_i,
         hls::stream<header_t> & header_in_o);

   void dbuff_egress(hls::stream<onboard_send_t> & onboard_send_i,
         hls::stream<onboard_send_t> & onboard_send_o,
         hls::stream<dbuff_in_raw_t> & dbuff_egress_i,
         hls::stream<srpt_dbuff_notif_t> & dbuff_notif_o,
         hls::stream<out_chunk_t> & out_chunk_i,
         hls::stream<out_chunk_t> & out_chunk_o, 
         hls::stream<dma_r_req_raw_t> & dma_r_req_o);
} 
#endif
