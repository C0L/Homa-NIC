#ifndef LINK_H
#define LINK_H

#include "homa.hh"
#include "srptmgmt.hh"
#include "rpcmgmt.hh"

void next_pkt_selector(hls::stream<srpt_data_send_t> & data_pkt_i,
		       hls::stream<srpt_grant_send_t> & grant_pkt_i,
		       hls::stream<header_t> & out_pkt_o);

void pkt_builder(hls::stream<header_t> & header_out_i,
		 hls::stream<h2c_chunk_t> & chunk_out_o);

void pkt_chunk_egress(hls::stream<h2c_chunk_t> & out_chunk_i,
		      hls::stream<raw_stream_t> & link_egress);

void pkt_chunk_ingress(hls::stream<raw_stream_t> & link_ingress,
		       hls::stream<header_t> & header_in_o,
		       hls::stream<c2h_chunk_t> & chunk_in_o);

#endif
