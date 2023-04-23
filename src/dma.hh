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

//struct args_t {
//  params_t params;
//  xmit_buffer_t buffer;
//};

void dma_ingress(homa_t * homa,
		 params_t * params,
		 hls::burst_maxi<xmit_mblock_t> maxi,
		 hls::stream<xmit_in_t> & xmit_buffer_insert,
		 hls::stream<xmit_id_t> & xmit_stack_next,
		 hls::stream<homa_rpc_id_t> & rpc_stack_next,
		 hls::stream<hashpack_t> & rpc_table_request,
		 hls::stream<homa_rpc_id_t> & rpc_table_response,
		 hls::stream<homa_rpc_id_t> & rpc_buffer_request,
		 hls::stream<homa_rpc_t> & rpc_buffer_response,
		 hls::stream<homa_rpc_t> & rpc_buffer_insert,
		 hls::stream<srpt_entry_t> & srpt_queue_insert);


#endif
