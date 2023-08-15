#ifndef LINK_H
#define LINK_H

#include "homa.hh"
#include "srptmgmt.hh"
#include "rpcmgmt.hh"

void egress_selector(hls::stream<srpt_pktq_t> & data_pkt_i,
		     hls::stream<srpt_grant_out_t> & grant_pkt_i,
		     hls::stream<header_t> & out_pkt_o);

void pkt_builder(hls::stream<header_t> & header_out_i,
		 hls::stream<out_chunk_t> & chunk_out_o);




//#ifdef STEPPED
void pkt_chunk_egress(bool egress_en,
		      hls::stream<out_chunk_t> & out_chunk_i,
		      hls::stream<raw_stream_t> & link_egress);

//#else
//void pkt_chunk_egress(hls::stream<out_chunk_t> & out_chunk_i,
//		      hls::stream<raw_stream_t> & link_egress);
//#endif

//#ifdef STEPPED
void pkt_chunk_ingress(bool ingress_en,
		       hls::stream<raw_stream_t> & link_ingress,
		       hls::stream<header_t> & header_in_o,
		       hls::stream<in_chunk_t> & chunk_in_o);
//#else
//void pkt_chunk_ingress(hls::stream<raw_stream_t> & link_ingress,
//		       hls::stream<header_t> & header_in_o,
//		       hls::stream<in_chunk_t> & chunk_in_o);
//#endif

#endif
