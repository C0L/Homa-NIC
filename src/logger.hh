#ifndef LOGGER_H
#define LOGGER_H

#include "homa.hh"

void logger(hls::stream<log_status_t> & dma_read_log_i,
	    hls::stream<log_status_t> & dma_write_log_i,
	    hls::stream<log_entry_t>  & log_out_o);

#endif

