#include <string.h>

#include "hls_math.h"
#include "ap_int.h"
#include "dma.hh"
#include "rpcmgmt.hh"
#include "srptmgmt.hh"

void homa_recvmsg(homa_t * homa,
		  recvmsg_t * recvmsg,
		  hls::stream<recvmsg_t> & recvmsg_o) {
  if (recvmsg->valid) {
    recvmsg_t new_recvmsg = *recvmsg;
    recvmsg_o.write(new_recvmsg);
  }
}

/**
 * sendmsg() - Injests new RPCs and homa configurations into the
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
void homa_sendmsg(homa_t * homa,
	     sendmsg_t * sendmsg,
	     hls::stream<dma_r_req_t> & dma_read_o,
	     hls::stream<dbuff_id_t> & freed_rpcs_i,
	     hls::stream<sendmsg_t> & sendmsg_o) {

  // TODO could also move this to another core that just sends its outputs to a FIFO here...
  static stack_t<dbuff_id_t, NUM_DBUFF> dbuff_stack(true);

  DEBUG_MSG("DMA Ingress") 

  /*
    TODO ensure this FIFO is very long a this process may not activate frequently
    and it could cause a stall if someone is trying to write a freed dbuff id to
    the free stream
  */

  if (!freed_rpcs_i.empty()) {
    dbuff_id_t dbuff_id = freed_rpcs_i.read();
    dbuff_stack.push(dbuff_id);
  }

  DEBUG_MSG(sendmsg->length)
  if (sendmsg->valid)  {
    float chunks = ((float) sendmsg->length) / DBUFF_CHUNK_SIZE;
    uint32_t num_chunks = ceil(chunks);

    DEBUG_MSG(num_chunks)

    dbuff_id_t dbuff_id = dbuff_stack.pop();

    sendmsg_t new_sendmsg = *sendmsg;
    new_sendmsg.dbuff_id = dbuff_id;
    new_sendmsg.granted = (homa->rtt_bytes > new_sendmsg.length) ? new_sendmsg.length : homa->rtt_bytes;

    sendmsg_o.write(new_sendmsg);

    for (uint32_t i = 0; i < num_chunks; ++i) {
      dma_read_o.write({new_sendmsg.buffin + i, dbuff_id});
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

  if (!rpc_ingress__dma_read.empty()) {
    dma_r_req_t dma_req = rpc_ingress__dma_read.read();
    dma_requests__dbuff.write({*(maxi + dma_req.offset), dma_req.dbuff_id, dma_req.offset});
  }
}

/**
 * dma_read() - Queue requests on the MAXI bus for RPC message data
 * @maxi - The MAXI interface connected to DMA space
 * @dma_requests__dbuff - 64B data chunks for storage in data space
 */
void dma_write(char * maxi,
//void dma_write(dbuff_chunk_t * maxi,
	       hls::stream<dma_w_req_t> & dbuff_ingress__dma_write) {

#pragma HLS pipeline II=1

  if (!dbuff_ingress__dma_write.empty()) {
    //std::cerr << "DMA WRITE\n";
    dma_w_req_t dma_req = dbuff_ingress__dma_write.read();
    std::cout << dma_req.offset << std::endl;
    std::cout << std::hex << dma_req.block << std::endl;
    *((dbuff_chunk_t*) (maxi + dma_req.offset)) = dma_req.block;
  }
}
