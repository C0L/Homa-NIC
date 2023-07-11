#include "dma.hh"

/*
 * homa_recvmsg() - Primes the core to accept data from an RPC
 * ...
 */
void homa_recvmsg(hls::stream<hls::axis<recvmsg_t, 0, 0, 0>> & recvmsg_i,
                  hls::stream<recvmsg_t> & recvmsg_o) {
   // TODO this used to perform some actual functions. Will keep this here
   // until it for sure is no longer needed
   hls::axis<recvmsg_t,0 ,0 ,0> recvmsg = recvmsg_i.read();
   recvmsg_o.write(recvmsg.data);
}

/* TODO This is not outdated
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
void homa_sendmsg(hls::stream<hls::axis<sendmsg_t, 0, 0, 0>> & sendmsg_i,
                  hls::stream<sendmsg_t> & sendmsg_o,
                  hls::stream<sendmsg_t> & dummy) {
   hls::axis<sendmsg_t, 0, 0, 0> sendmsg = sendmsg_i.read(); 
   sendmsg.data.granted = (sendmsg.data.rtt_bytes > sendmsg.data.length) ? sendmsg.data.length : sendmsg.data.rtt_bytes;
   sendmsg_o.write(sendmsg.data);
   dummy.write(sendmsg.data);
}


// TODO MOVING TO A SEPERATE CORE
/*
 * dma_read() - Queue requests on the MAXI bus for RPC message data
 * @maxi - The MAXI interface connected to DMA space
 * @dma_requests__dbuff - 64B data chunks for storage in data space
 */
// void dma_read(
//       hls::stream<dma_r_req_t, VERIF_DEPTH> & rpc_ingress__dma_read,
//       hls::stream<dbuff_in_t, VERIF_DEPTH> & dma_requests__dbuff) {
// 
//    static dma_r_req_t dma_req;
//    static uint32_t chunk = 0;
// 
//    static uint32_t dma_count = 0;
//    while (dma_count < homa_cfg.dma_count) {
// 
// #pragma HLS pipeline II=1 style=flp
// 
//       if (dma_req.length == 0 && rpc_ingress__dma_read.read_nb(dma_req)) {
//          // if (rpc_ingress__dma_read.read_nb(dma_req) && dma_req.length == 0) {
//          dma_count++;
//          chunk = 0;
//       }
// 
//       if (dma_req.length != 0) {
//          std::cerr << "offset " << dma_req.offset << std::endl;
//          std::cerr << "length " << dma_req.length << std::endl;
//          std::cerr << "chunk " << chunk << std::endl;
//          integral_t big_order = *((integral_t*) (maxi + dma_req.offset + chunk * 64));
//          dma_requests__dbuff.write_nb({big_order, dma_req.dbuff_id, dma_req.offset + chunk});
//          dma_req.length = (dma_req.length < 1) ? (ap_uint<32>) 0 : (ap_uint<32>) (dma_req.length - 1);
//          chunk++;
//       }
//       }
//    }

/**
 * dma_write() - Queue requests on the MAXI bus for RPC message data
 * @maxi - The MAXI interface connected to DMA space
 * @dma_requests__dbuff - 64B data chunks for storage in data space
 */
//void dma_write(char * maxi,
//      homa_t homa_cfg,
//      hls::stream<dma_w_req_t, VERIF_DEPTH> & dbuff_ingress__dma_write) {
//   //TODO this is not really pipelined? 70 cycle layency per req
//
//   static uint32_t dma_count = 0;
//
//   while (dma_count < homa_cfg.dma_count) {
//#pragma HLS pipeline II=1 style=flp
//      dma_w_req_t dma_req;
//      if (dbuff_ingress__dma_write.read_nb(dma_req)) {
//
//         std::cerr << "DMA WRITE: " << dma_req.offset << std::endl;
//         *((integral_t*) (maxi + dma_req.offset)) = dma_req.block;
//         std::cerr << "DMA WRITE COMPLETE " << dma_req.offset << std::endl;
//         dma_count++;
//      }
//   }
//}
