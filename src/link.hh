#ifndef LINK_H
#define LINK_H

#include "homa.hh"
#include "srptmgmt.hh"
#include "rpcmgmt.hh"
#include "dma.hh"
#include "timer.hh"

// void byte_swap(ap_uint<512> & in, ap_uint<512> & out);

void egress_selector(hls::stream<ready_data_pkt_t> & data_pkt_i,
		     hls::stream<ap_uint<51>> & grant_pkt_i,
		     hls::stream<rexmit_t> & rexmit_pkt_i,
		     hls::stream<header_t> & out_pkt_o);

void pkt_chunk_egress(hls::stream<header_t> & rpc_state__chunk_dispatch,
		      hls::stream<out_chunk_t> & chunk_dispatch__dbuff);


void pkt_chunk_ingress(hls::stream<raw_stream_t> & link_ingress,
		       hls::stream<header_t> & chunk_ingress__peer_map,
		       hls::stream<in_chunk_t> & chunk_ingress__dbuff_ingress);



#endif
