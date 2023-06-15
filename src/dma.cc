#include <string.h>


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
	     hls::stream<sendmsg_t> & sendmsg_o) {

  if (sendmsg->valid)  {
    sendmsg_t new_sendmsg = *sendmsg;
    
    new_sendmsg.granted = (homa->rtt_bytes > new_sendmsg.length) ? new_sendmsg.length : homa->rtt_bytes;

    //std::cerr << new_sendmsg.granted << std::endl;

    sendmsg_o.write(new_sendmsg);
  }
}

// https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/AXI-Burst-Transfers

/**
 * dma_read() - Queue requests on the MAXI bus for RPC message data
 * @maxi - The MAXI interface connected to DMA space
 * @dma_requests__dbuff - 64B data chunks for storage in data space
 */
void dma_read(char * maxi,
	      hls::stream<dma_r_req_t> & rpc_ingress__dma_read,
	      hls::stream<dbuff_in_t> & dma_requests__dbuff) {

  if (!rpc_ingress__dma_read.empty()) {
    dma_r_req_t dma_req = rpc_ingress__dma_read.read();
    for (int i = 0; i < dma_req.length; ++i) {
#pragma HLS pipeline II=1
      //std::cout << "BLOCK: " << std::hex << *((dbuff_chunk_t*) (maxi + dma_req.offset + (i*64))) << std::endl;
      dbuff_chunk_t chunk = *((dbuff_chunk_t*) (maxi + dma_req.offset + (i*64)));
      //dbuff_chunk_t chunk;
      //memcpy(&chunk, maxi + dma_req.offset + (i*64), 64);
      dma_requests__dbuff.write({chunk, dma_req.dbuff_id, dma_req.offset + i});
    }
  }
}

/**
 * dma_read() - Queue requests on the MAXI bus for RPC message data
 * @maxi - The MAXI interface connected to DMA space
 * @dma_requests__dbuff - 64B data chunks for storage in data space
 */
void dma_write(char * maxi,
	       hls::stream<dma_w_req_t> & dbuff_ingress__dma_write) {

#pragma HLS pipeline II=1

  if (!dbuff_ingress__dma_write.empty()) {
    dma_w_req_t dma_req = dbuff_ingress__dma_write.read();
    *((dbuff_chunk_t*) (maxi + dma_req.offset)) = dma_req.block;
  }
}
