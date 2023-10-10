#ifndef LOGGER_H
#define LOGGER_H

#include "homa.hh"


void logger(hls::stream<ap_uint<8>> & dma_w_req_log_i,
	    hls::stream<ap_uint<8>> & dma_w_stat_log_i, 
	    hls::stream<ap_uint<8>> & dma_r_req_log_i,
	    hls::stream<ap_uint<8>> & dma_r_read_log_i,
	    hls::stream<ap_uint<8>> & dma_r_stat_log_i,
	    hls::stream<ap_uint<8>> & h2c_pkt_log_i,
	    hls::stream<ap_uint<8>> & c2h_pkt_log_i,
	    hls::stream<ap_uint<8>> & dbuff_notif_log_i,
	    hls::stream<log_entry_t> & log_out_o);

#endif

