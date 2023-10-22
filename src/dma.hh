#ifndef DMA_H
#define DMA_H

#include "homa.hh"

void dma_read(hls::stream<am_cmd_t> & cmd_queue_o,
	      hls::stream<ap_axiu<512,0,0,0>> & data_queue_i,
	      hls::stream<am_status_t> & status_queue_i,
	      hls::stream<dma_r_req_t> & dma_req_i,
	      hls::stream<h2c_dbuff_t> & dbuff_in_o,
              hls::stream<ap_uint<8>> & dma_req_log_o,
              hls::stream<ap_uint<8>> & dma_read_log_o,
              hls::stream<ap_uint<8>> & dma_stat_log_o);

void dma_write(hls::stream<am_cmd_t> & cmd_queue_o,
	       hls::stream<ap_axiu<512,0,0,0>> & data_queue_o,
	       hls::stream<am_status_t> & status_queue_i,
	       hls::stream<dma_w_req_t> & dma_w_req_data_i,
	       hls::stream<dma_w_req_t> & dma_w_req_sendmsg_i,
	       hls::stream<dma_w_req_t> & dma_w_req_recvmsg_i,
	       hls::stream<ap_uint<8>> & dma_req_log_o,
	       hls::stream<ap_uint<8>> & dma_stat_log_o);

#endif
