#ifndef LINK_H
#define LINK_H

#include "homa.hh"
#include "srptmgmt.hh"
#include "rpcmgmt.hh"
extern "C"{
   void egress_selector(hls::stream<srpt_pktq_t> & data_pkt_i,
         hls::stream<srpt_grant_out_t> & grant_pkt_i,
         hls::stream<header_t> & out_pkt_o);

   void pkt_builder(hls::stream<header_t> & header_out_i,
         hls::stream<out_chunk_t> & chunk_out_o);

   void pkt_chunk_egress(hls::stream<out_chunk_t> & out_chunk_i,
			 hls::stream<raw_stream_t, STREAM_DEPTH> & link_egress);

    void pkt_chunk_ingress(hls::stream<raw_stream_t, STREAM_DEPTH> & link_ingress,
         hls::stream<header_t> & chunk_ingress__peer_map,
         hls::stream<in_chunk_t> & chunk_ingress__dbuff_ingress);
}

#endif
