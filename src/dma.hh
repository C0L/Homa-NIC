#ifndef DMA_H
#define DMA_H

#include <hls_stream.h>
#include <stdint.h>

#include "net.hh"
#include "homa.hh"
#include "xmitbuff.hh"
#include "rpcmgmt.hh"
#include "srptmgmt.hh"

struct homa_t;

struct params_t {
  // Offset in DMA space for output
  uint32_t buffout;
  // Offset in DMA space for input
  uint32_t buffin;
  int length;
  sockaddr_in6_t dest_addr;
  uint64_t id;
  uint64_t completion_cookie;
};

struct dma_egress_req_t {
  int offset;
  xmit_mblock_t mblock;
};



void dma_ingress(homa_t * homa,
		 params_t * params,
		 hls::burst_maxi<xmit_mblock_t> maxi,
		 hls::stream<xmit_in_t> & xmit_buffer_insert,
		 hls::stream<xmit_id_t> & xmit_stack_next,
		 hls::stream<rpc_id_t> & rpc_stack_next,
		 hls::stream<rpc_hashpack_t> & rpc_table_request,
		 hls::stream<rpc_id_t> & rpc_table_response,
		 hls::stream<rpc_id_t> & rpc_buffer_request,
		 hls::stream<homa_rpc_t> & rpc_buffer_response,
		 hls::stream<homa_rpc_t> & rpc_buffer_insert,
		 hls::stream<srpt_xmit_entry_t> & srpt_queue_insert,
		 hls::stream<peer_id_t> & peer_stack_next,
		 hls::stream<peer_hashpack_t> & peer_table_request,
		 hls::stream<peer_id_t> & peer_table_response,
		 hls::stream<homa_peer_t> & peer_table_insert,
		 hls::stream<homa_peer_t> & peer_buffer_insert,
		 hls::stream<ap_uint<1>> & timer_request,
		 hls::stream<ap_uint<64>> & timer_response);

void dma_egress(hls::stream<dma_egress_req_t> & dma_egress_reqs,
		xmit_mblock_t * maxi_out);

#endif
