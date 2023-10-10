#ifndef DATABUFF_H
#define DATABUFF_H

#include "homa.hh"

void c2h_databuff(hls::stream<c2h_chunk_t> & chunk_in_o,
		  hls::stream<dma_w_req_t> & dma_w_req_o,
		  hls::stream<header_t> & header_in_i,
		  hls::stream<header_t> & header_in_o);

void h2c_databuff(hls::stream<h2c_dbuff_t> & dbuff_egress_i,
		  hls::stream<srpt_queue_entry_t> & dbuff_notif_o,
		  hls::stream<h2c_chunk_t> & out_chunk_i,
		  hls::stream<h2c_chunk_t> & out_chunk_o,
		  hls::stream<ap_uint<8>> & dbuff_notif_log_o);

#endif
