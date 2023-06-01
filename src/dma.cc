#include <string.h>

#include "ap_int.h"
#include "dma.hh"
#include "rpcmgmt.hh"
#include "srptmgmt.hh"

/**
 * dma_ingress()
 * Pull user request from DMA and the first message caches worth of bytes
 */
void dma_ingress(homa_t * homa,
		 params_t * params,
		 hls::burst_maxi<dbuff_block_t> maxi,
		 hls::stream<dbuff_id_t> & freed_rpcs_o,
		 hls::stream<dbuff_in_t> & dbuff_o,
		 hls::stream<new_rpc_t> & new_rpc_o) {
#pragma HLS interface mode=ap_ctrl_hs port=return

  static stack_t<dbuff_id_t, NUM_DBUFF> dbuff_stack(true);

  if (!freed_rpcs_o.empty()) {
    dbuff_id_t dbuff_id = freed_rpcs_o.read();
    dbuff_stack.push(dbuff_id);
  }

  dbuff_id_t dbuff_id = dbuff_stack.pop();

  // AXI burst request
  maxi.read_request(params->buffin, DBUFF_NUM_CHUNKS);

  // Iteratively read result of burst and set to xmit buffer
  for (int i = 0; i < DBUFF_NUM_CHUNKS; i++) {
#pragma HLS pipeline II=1
    dbuff_in_t dbuff_in = {maxi.read(), dbuff_id};
    dbuff_o.write(dbuff_in);
  }

  new_rpc_t new_rpc;
  new_rpc.dbuff_id = dbuff_id;
  new_rpc.buffout = params->buffout;
  new_rpc.buffin = params->buffin;
  new_rpc.rtt_bytes = homa->rtt_bytes;
  new_rpc.length = params->length;
  new_rpc.dest_addr = params->dest_addr;
  new_rpc.src_addr = params->src_addr;
  new_rpc.id = params->id;
  new_rpc.completion_cookie = params->completion_cookie;

  new_rpc_o.write(new_rpc);
}




void dma_egress(hls::stream<dma_egress_req_t> & dma_egress_reqs,
		dbuff_block_t * maxi_out) {
  // TODO this is a auto-restarting kernel
#pragma HLS INTERFACE mode=ap_ctrl_none port=return

  dma_egress_req_t dma_egress_req;
  dma_egress_reqs.read(dma_egress_req);

  *(maxi_out + dma_egress_req.offset) = dma_egress_req.mblock;

}
