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

struct dma_egress_req_t {
  uint32_t offset;
  dbuff_chunk_t mblock;
};

void dma_ingress(homa_t * homa,
		 params_t * params,
		 hls::burst_maxi<dbuff_chunk_t> maxi,
		 hls::stream<dbuff_id_t> & freed_rpcs_o,
		 hls::stream<dbuff_in_t> & dbuff_o,
		 hls::stream<new_rpc_t> & new_rpc_o);

void dma_egress(hls::stream<dma_egress_req_t> & dma_egress_reqs,
		dbuff_chunk_t * maxi_out);

#endif
