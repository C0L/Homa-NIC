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

void new_rpc(homa_t * homa,
	     params_t * params,
	     hls::stream<dma_r_req_t> & rpc_ingress__dma_read,
	     hls::stream<dbuff_id_t> & freed_rpcs_o,
	     hls::stream<new_rpc_t> & new_rpc_o);

void dma_read(dbuff_chunk_t * maxi,
	      hls::stream<dma_r_req_t> & rpc_ingress__dma_read,
	      hls::stream<dbuff_in_t> & dma_requests__dbuff);

void dma_write(dbuff_chunk_t * maxi,
	       hls::stream<dma_w_req_t> & dbuff_ingress__dma_write);

#endif
