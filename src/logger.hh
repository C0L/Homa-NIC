#ifndef LOGGER_H
#define LOGGER_H

#include "homa.hh"

void logger(hls::stream<ap_uint<64>> & dma_read_log_i,
	    hls::stream<ap_uint<64>> & dma_write_log_i,
	    hls::stream<log_entry_t>  & log_out_o);

#endif

