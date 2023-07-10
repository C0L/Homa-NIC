#ifndef DMA_H
#define DMA_H

#include <hls_stream.h>
#include <stdint.h>

// #include "net.hh"
#include "homa.hh"
// #include "databuff.hh"
// #include "rpcmgmt.hh"
// #include "srptmgmt.hh"

// struct homa_t;

void homa_sendmsg(const homa_t homa,
		  const sendmsg_t params,
		  hls::stream<sendmsg_t, VERIF_DEPTH> & sendmsg_o);

void homa_recvmsg(const homa_t homa,
		  const recvmsg_t recvmsg,
		  hls::stream<recvmsg_t, VERIF_DEPTH> & recvmsg_o);

// void byte_order_flip(dbuff_chunk_t & in, dbuff_chunk_t & out);

void dma_read(char * maxi,
	      hls::stream<dma_r_req_t, VERIF_DEPTH> & rpc_ingress__dma_read,
	      hls::stream<dbuff_in_t, VERIF_DEPTH> & dma_requests__dbuff);

void dma_write(char * maxi,
	       hls::stream<dma_w_req_t, VERIF_DEPTH> & dbuff_ingress__dma_write);

#endif
