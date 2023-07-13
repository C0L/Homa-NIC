#ifndef LINK_H
#define LINK_H

#include "homa.hh"
#include "srptmgmt.hh"
#include "rpcmgmt.hh"
extern "C"{
   void egress_selector(hls::stream<ready_data_pkt_t, VERIF_DEPTH> & data_pkt_i,
         hls::stream<ap_uint<51>, VERIF_DEPTH> & grant_pkt_i,
         hls::stream<header_t, VERIF_DEPTH> & out_pkt_o);

   void pkt_builder(hls::stream<header_t, VERIF_DEPTH> & header_out_i,
         hls::stream<out_chunk_t, VERIF_DEPTH> & chunk_out_o);

   void pkt_chunk_egress(hls::stream<out_chunk_t> & out_chunk_i,
         hls::stream<raw_stream_t, VERIF_DEPTH> & link_egress);

   void pkt_chunk_ingress(hls::stream<raw_stream_t, VERIF_DEPTH> & link_ingress,
         hls::stream<header_t, VERIF_DEPTH> & chunk_ingress__peer_map,
         hls::stream<in_chunk_t, VERIF_DEPTH> & chunk_ingress__dbuff_ingress);
}


#endif
