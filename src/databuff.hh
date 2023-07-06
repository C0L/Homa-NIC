#ifndef DATABUFF_H
#define DATABUFF_H

#include "homa.hh"
#include "net.hh"
#include "dma.hh"

void dbuff_stack(hls::stream<sendmsg_t, VERIF_DEPTH> & sendmsg_i,
		 hls::stream<sendmsg_t, VERIF_DEPTH> & sendmsg_o,
		 hls::stream<dma_r_req_t, VERIF_DEPTH> & dma_read_o);

void dbuff_ingress(hls::stream<in_chunk_t, VERIF_DEPTH> & chunk_ingress__dbuff_ingress,
		   hls::stream<dma_w_req_t, VERIF_DEPTH> & dbuff_ingress__dma_write,
		   hls::stream<header_t, VERIF_DEPTH> & rpc_store__dbuff_ingress);

void dbuff_egress(hls::stream<dbuff_in_t, VERIF_DEPTH> & dbuff_egress_i,
		  hls::stream<dbuff_notif_t, VERIF_DEPTH> & dbuff_egress__srpt_data,
		  hls::stream<out_chunk_t, VERIF_DEPTH> & chunk_dispatch__dbuff_egress,
		  hls::stream<raw_stream_t> & link_egress);

#endif
