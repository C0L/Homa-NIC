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
  dbuff_block_t mblock;
};

void dma_ingress(homa_t * homa,
		 params_t * params,
		 hls::burst_maxi<dbuff_block_t> maxi,
		 hls::stream<dbuff_id_t> & freed_rpcs_o,
		 hls::stream<dbuff_in_t> & dbuff_o,
		 hls::stream<new_rpc_t> & new_rpc_o);


//void dma_ingress(homa_t * homa,
//		 params_t * params,
//		 hls::burst_maxi<xmit_mblock_t> maxi,
//		 hls::stream<xmit_in_t> & xmit_buffer_insert,
//		 hls::stream<xmit_id_t, 2> & xmit_stack_next,
//		 hls::stream<rpc_id_t> & rpc_stack_next,
//		 hls::stream<rpc_hashpack_t> & rpc_table_request,
//		 hls::stream<rpc_id_t> & rpc_table_response,
//		 hls::stream<rpc_id_t> & rpc_buffer_request,
//		 hls::stream<homa_rpc_t> & rpc_buffer_response,
//		 hls::stream<homa_rpc_t> & rpc_buffer_insert,
//		 hls::stream<srpt_xmit_entry_t> & srpt_queue_insert,
//		 hls::stream<peer_id_t> & peer_stack_next,
//		 hls::stream<peer_hashpack_t> & peer_table_request,
//		 hls::stream<peer_id_t> & peer_table_response,
//		 hls::stream<homa_peer_t> & peer_table_insert,
//		 hls::stream<homa_peer_t> & peer_buffer_insert);

void dma_egress(hls::stream<dma_egress_req_t> & dma_egress_reqs,
		dbuff_block_t * maxi_out);

#endif
