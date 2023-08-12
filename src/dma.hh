#ifndef DMA_H
#define DMA_H

#include "homa.hh"

void dma_read(const integral_t * maxi,
	      hls::stream<srpt_sendq_t> & dma_req_i,
	      hls::stream<dbuff_in_t> & dbuff_in_o);

void dma_write(integral_t * maxi,
	       hls::stream<dma_w_req_t> & dma_w_req_i);

#endif
