#include <string.h>

#include "hls_math.h"
#include "ap_int.h"
#include "dma.hh"
#include "rpcmgmt.hh"
#include "srptmgmt.hh"

/**
 * new_rpc() - Injests new RPCs and homa configurations into the
 * homa processor. Manages the databuffer IDs, as those must be generated before
 * started pulling data from DMA, and we need to know the ID to store it in the
 * homa_rpc.
 * @homa         - Homa configuration parameters
 * @params       - New RPCs that need to be onboarded
 * @maxi         - Burstable AXI interface to the DMA space
 * @freed_rpcs_o - Free RPC data buffer IDs for inclusion into the new RPCs
 * @dbuff_0      - Output to the data buffer for placing the chunks from DMA
 * @new_rpc_o    - Output for the next step of the new_rpc injestion
 */
void new_rpc(homa_t * homa,
	     params_t * params,
	     hls::stream<dma_r_req_t> & new_rpc__dma_read,
	     hls::stream<dbuff_id_t> & freed_rpcs_o,
	     hls::stream<new_rpc_t> & new_rpc_o) {

  // TODO could also move this to another core that just sends its outputs to a FIFO here...
  static stack_t<dbuff_id_t, NUM_DBUFF> dbuff_stack(true);

  DEBUG_MSG("DMA Ingress") 

  /*
    TODO ensure this FIFO is very long a this process may not activate frequently
    and it could cause a stall if someone is trying to write a freed dbuff id to
    the free stream
  */

  if (!freed_rpcs_o.empty()) {
    dbuff_id_t dbuff_id = freed_rpcs_o.read();
    dbuff_stack.push(dbuff_id);
  }

  DEBUG_MSG(params->length)

  if (params->valid)  {
    float chunks = ((float) params->length) / DBUFF_CHUNK_SIZE;
    uint32_t num_chunks = ceil(chunks);

    std::cerr << num_chunks << std::endl;

    DEBUG_MSG(num_chunks)

    dbuff_id_t dbuff_id = dbuff_stack.pop();

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

    new_rpc_o.write(new_rpc);

    for (uint32_t i = 0; i < num_chunks; ++i) {
      new_rpc__dma_read.write({params->buffin + i, dbuff_id});
    }
  }
}

// https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/AXI-Burst-Transfers

/**
 * dma_read() - Queue requests on the MAXI bus for RPC message data
 * @maxi - The MAXI interface connected to DMA space
 * @dma_requests__dbuff - 64B data chunks for storage in data space
 */
void dma_read(dbuff_chunk_t * maxi,
	      hls::stream<dma_r_req_t> & rpc_ingress__dma_read,
	      hls::stream<dbuff_in_t> & dma_requests__dbuff) {

#pragma HLS pipeline II=1

  dma_r_req_t dma_req = rpc_ingress__dma_read.read();
  dma_requests__dbuff.write({*(maxi + dma_req.offset), dma_req.dbuff_id, dma_req.offset});
}

/**
 * dma_read() - Queue requests on the MAXI bus for RPC message data
 * @maxi - The MAXI interface connected to DMA space
 * @dma_requests__dbuff - 64B data chunks for storage in data space
 */
void dma_write(dbuff_chunk_t * maxi,
	       hls::stream<dma_w_req_t> & dbuff_ingress__dma_write) {

#pragma HLS pipeline II=1

  dma_w_req_t dma_req = dbuff_ingress__dma_write.read();
  *(maxi + dma_req.offset) = dma_req.block;
}
