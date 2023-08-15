#ifndef DMA_H
#define DMA_H

#include "homa.hh"

void dma(ap_uint<512> * maxi_in,
	 hls::stream<srpt_sendq_t> & dma_req_i,
	 hls::stream<dbuff_in_t> & dbuff_in_o,
	 ap_uint<512> * maxi_out,
	 hls::stream<dma_w_req_t> & dma_w_req_i);

void dma_read(integral_t * maxi,
	      bool maxi_read_en,
	      hls::stream<srpt_sendq_t> & dma_req_i,
	      hls::stream<dbuff_in_t> & dbuff_in_o);

void dma_write(integral_t * maxi,
	       bool maxi_write_en,
	       hls::stream<dma_w_req_t> & dma_w_req_i);

#endif
