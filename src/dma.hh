#ifndef DMA_H
#define DMA_H

#include <hls_stream.h>
#include <stdint.h>

#include "net.hh"
#include "homa.hh"
#include "databuff.hh"
#include "rpcmgmt.hh"
#include "srptmgmt.hh"

struct homa_t;

void homa_sendmsg(homa_t * homa,
		  sendmsg_t * params,
		  hls::stream<sendmsg_t> & sendmsg_o);

void homa_recvmsg(homa_t * homa,
		  recvmsg_t * recvmsg,
		  hls::stream<recvmsg_t> & recvmsg_o);

void byte_order_flip(dbuff_chunk_t & in, dbuff_chunk_t & out);

void dma_read(char * maxi,
	      hls::stream<dma_r_req_t> & rpc_ingress__dma_read,
	      hls::stream<dbuff_in_t> & dma_requests__dbuff);

void dma_write(char * maxi,
	       hls::stream<dma_w_req_t> & dbuff_ingress__dma_write);

#endif
