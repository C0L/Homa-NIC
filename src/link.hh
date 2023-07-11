#ifndef LINK_H
#define LINK_H

#include "homa.hh"
#include "srptmgmt.hh"
#include "rpcmgmt.hh"
extern "C"{
void egress_selector(hls::stream<ready_data_pkt_t, VERIF_DEPTH> & data_pkt_i,
		     hls::stream<ap_uint<51>, VERIF_DEPTH> & grant_pkt_i,
		     hls::stream<header_t, VERIF_DEPTH> & out_pkt_o);
}
extern "C"{
void pkt_chunk_egress(hls::stream<header_t, VERIF_DEPTH> & rpc_state__chunk_dispatch,
		      hls::stream<out_chunk_t, VERIF_DEPTH> & chunk_dispatch__dbuff);
}
extern "C"{
void pkt_chunk_ingress(hls::stream<raw_stream_t> & link_ingress,
		       hls::stream<header_t, VERIF_DEPTH> & chunk_ingress__peer_map,
		       hls::stream<in_chunk_t, VERIF_DEPTH> & chunk_ingress__dbuff_ingress);
}


#endif
