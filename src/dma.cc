#include <string.h>


#include "ap_int.h"
#include "dma.hh"
#include "rpcmgmt.hh"
#include "srptmgmt.hh"

void homa_recvmsg(const homa_t * homa,
      recvmsg_t * recvmsg,
      hls::stream<recvmsg_t, VERIF_DEPTH> & recvmsg_o) {
   if (recvmsg->valid && recvmsg_o.size() < VERIF_DEPTH) {
      recvmsg_t new_recvmsg = *recvmsg;
      //new_recvmsg.rtt_bytes = homa->rtt_bytes;
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
void homa_sendmsg(const homa_t * homa,
      sendmsg_t * sendmsg,
      hls::stream<sendmsg_t, VERIF_DEPTH> & sendmsg_o) {

   if (sendmsg->valid && sendmsg_o.size() < VERIF_DEPTH)  {
      sendmsg_t new_sendmsg = *sendmsg;

      new_sendmsg.granted = (homa->rtt_bytes > new_sendmsg.length) ? new_sendmsg.length : homa->rtt_bytes;

      sendmsg_o.write(new_sendmsg);
   }
}

/**
 * dma_read() - Queue requests on the MAXI bus for RPC message data
 * @maxi - The MAXI interface connected to DMA space
 * @dma_requests__dbuff - 64B data chunks for storage in data space
 */
void dma_read(char * maxi,
      hls::stream<dma_r_req_t, VERIF_DEPTH> & rpc_ingress__dma_read,
      hls::stream<dbuff_in_t, VERIF_DEPTH> & dma_requests__dbuff) {

   static dma_r_req_t dma_req;
   static uint32_t chunk = 0;

#pragma HLS pipeline II=1

   if (!rpc_ingress__dma_read.empty() && dma_req.length == 0) {
      dma_req = rpc_ingress__dma_read.read();
      chunk = 0;
   }

   // TODO this may be better in a loop. Harder to test though.

   if (dma_req.length != 0 && dma_requests__dbuff.size() < VERIF_DEPTH) {
      std::cerr << "offset " << dma_req.offset << std::endl;
      std::cerr << "length " << dma_req.length << std::endl;
      std::cerr << "chunk " << chunk << std::endl;
      integral_t big_order = *((integral_t*) (maxi + dma_req.offset + chunk * 64));
      dma_requests__dbuff.write({big_order, dma_req.dbuff_id, dma_req.offset + chunk});
      // dma_req.length = MAX((ap_uint<32>) (dma_req.length - 64), (ap_uint<32>) 0);
      dma_req.length = (dma_req.length < 1) ? (ap_uint<32>) 0 : (ap_uint<32>) (dma_req.length - 1);
      chunk++;
   }
}

/**
 * dma_write() - Queue requests on the MAXI bus for RPC message data
 * @maxi - The MAXI interface connected to DMA space
 * @dma_requests__dbuff - 64B data chunks for storage in data space
 */
void dma_write(char * maxi,
      hls::stream<dma_w_req_t, VERIF_DEPTH> & dbuff_ingress__dma_write) {
   //TODO this is not really pipelined? 70 cycle layency per req
#pragma HLS pipeline II=1
   dma_w_req_t dma_req;
   if (!dbuff_ingress__dma_write.empty()) {
      dma_req = dbuff_ingress__dma_write.read();

      std::cerr << "DMA WRITE: " << dma_req.offset << std::endl;
      *((integral_t*) (maxi + dma_req.offset)) = dma_req.block;
      std::cerr << "DMA WRITE COMPLETE " << dma_req.offset << std::endl;
   }
}
