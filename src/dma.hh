#ifndef DMA_H
#define DMA_H

#include "homa.hh"

//void dma(ap_uint<512> * maxi_in,
//	 hls::stream<srpt_sendq_t> & dma_req_i,
//	 hls::stream<dbuff_in_t> & dbuff_in_o,
//	 ap_uint<512> * maxi_out,
//	 hls::stream<dma_w_req_t> & dma_w_req_i);

//#ifdef STEPPED
void dma_read(bool dma_read_en,
	      volatile ap_uint<512> * maxi,
	      hls::stream<srpt_sendq_t> & dma_req_i,
	      hls::stream<dbuff_in_t> & dbuff_in_o);
//#else
//void dma_read(ap_uint<512> * maxi,
//	      hls::stream<srpt_sendq_t> & dma_req_i,
//	      hls::stream<dbuff_in_t> & dbuff_in_o);
//#endif

//#ifdef STEPPED
void dma_write(bool dma_write_en,
	       volatile ap_uint<512> * maxi,
	       hls::stream<dma_w_req_t> & dma_w_req_i);
//#else
//void dma_write(ap_uint<512> * maxi,
//	       hls::stream<dma_w_req_t> & dma_w_req_i);
//#endif

#endif
