#ifndef DATABUFF_H
#define DATABUFF_H

#include "homa.hh"
#include "net.hh"
#include "dma.hh"

void dbuff_stack(hls::stream<sendmsg_t> & sendmsg_i,
		 hls::stream<sendmsg_t> & sendmsg_o,
		 hls::stream<dma_r_req_t> & dma_read_o);

void dbuff_ingress(hls::stream<in_chunk_t> & chunk_ingress__dbuff_ingress,
		   hls::stream<dma_w_req_t> & dbuff_ingress__dma_write,
		   hls::stream<header_t> & rpc_store__dbuff_ingress);

void dbuff_egress(hls::stream<dbuff_in_t> & dbuff_egress_i,
		  hls::stream<dbuff_notif_t> & dbuff_egress__srpt_data,
		  hls::stream<out_chunk_t> & chunk_dispatch__dbuff_egress,
		  hls::stream<raw_stream_t> & link_egress);
		  


#endif
