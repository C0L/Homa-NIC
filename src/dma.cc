#include <string.h>

#include "hls_math.h"
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
		 hls::burst_maxi<dbuff_chunk_t> maxi,
		 hls::stream<dbuff_id_t> & freed_rpcs_o,
		 hls::stream<dbuff_in_t> & dbuff_o,
		 hls::stream<new_rpc_t> & new_rpc_o) {
#pragma HLS interface mode=ap_ctrl_hs port=return

  static stack_t<dbuff_id_t, NUM_DBUFF> dbuff_stack(true);

  DEBUG_MSG("DMA Ingress") 

  if (!freed_rpcs_o.empty()) {
    dbuff_id_t dbuff_id = freed_rpcs_o.read();
    dbuff_stack.push(dbuff_id);
  }

  DEBUG_MSG(params->length)

  double chunks = params->length / DBUFF_CHUNK_SIZE;
  uint32_t num_chunks = ceil(chunks);

  DEBUG_MSG(num_chunks)

  // AXI burst request
  maxi.read_request(params->buffin, num_chunks);

  dbuff_id_t dbuff_id = dbuff_stack.pop();
					    
  DEBUG_MSG(dbuff_id);

  dbuff_in_t dbuff_in = {maxi.read(), dbuff_id, 0};
  dbuff_o.write(dbuff_in);
  
  for (int i = 1; i < num_chunks; i++) {
#pragma HLS pipeline II=1
    dbuff_in = {maxi.read(), dbuff_id, i};
    dbuff_o.write(dbuff_in);
  }

  // TODO only really need to wait for the first read to suceeed before this can be launched
  new_rpc_t new_rpc;
  new_rpc.dbuff_id = dbuff_id;
  new_rpc.buffout = params->buffout;
  new_rpc.buffin = params->buffin;
  new_rpc.rtt_bytes = homa->rtt_bytes;
  new_rpc.length = params->length;

  new_rpc.granted = (new_rpc.rtt_bytes > new_rpc.length) ? new_rpc.length : new_rpc.rtt_bytes;

  new_rpc.dest_addr = params->dest_addr;
  new_rpc.src_addr = params->src_addr;
  new_rpc.id = params->id;
  new_rpc.completion_cookie = params->completion_cookie;

  //dbuff_o.write(dbuff_in);

  //  dbuff_in_t dbuff_in = {maxi.read(), dbuff_id};
  //  dbuff_o.write(dbuff_in);
  //
  new_rpc_o.write(new_rpc);
  //
  //  // Iteratively read result of burst and set to xmit buffer
  //
  //  // TODO does the 64 byte chunk influence the retrieval of packet data?
  //  // TODO should be handled by some async process. Let the dbuff handle rebuffering packets where data has not yet arrived.
  //  for (int i = 0; i < DBUFF_NUM_CHUNKS-1; i++) {
  //#pragma HLS pipeline II=1
  //    dbuff_in = {maxi.read(), dbuff_id};
  //    dbuff_o.write(dbuff_in);
  //  }
}




void dma_egress(hls::stream<dma_egress_req_t> & dma_egress_reqs,
		dbuff_chunk_t * maxi_out) {
  // TODO this is a auto-restarting kernel
#pragma HLS INTERFACE mode=ap_ctrl_none port=return

  dma_egress_req_t dma_egress_req;
  dma_egress_reqs.read(dma_egress_req);

  *(maxi_out + dma_egress_req.offset) = dma_egress_req.mblock;

}
