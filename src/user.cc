#include "user.hh"

extern "C" {

   /*
    * homa_recvmsg() - Primes the core to accept data from an RPC
    * ...
    */
   void homa_recvmsg(hls::stream<recvmsg_raw_t> & recvmsg_i,
         hls::stream<recvmsg_t> & recvmsg_o) {
#pragma HLS pipeline II=1

      recvmsg_raw_t recvmsg_raw = recvmsg_i.read();
   
      recvmsg_t recvmsg;
      recvmsg.buffout   = recvmsg_raw(RECVMSG_BUFFOUT);
      recvmsg.saddr     = recvmsg_raw(RECVMSG_SADDR);
      recvmsg.daddr     = recvmsg_raw(RECVMSG_DADDR); 
      recvmsg.sport     = recvmsg_raw(RECVMSG_SPORT);
      recvmsg.dport     = recvmsg_raw(RECVMSG_DPORT);
      recvmsg.id        = recvmsg_raw(RECVMSG_ID);
      recvmsg.rtt_bytes = recvmsg_raw(RECVMSG_RTT);
         
      recvmsg_o.write(recvmsg);
   }

   /* TODO This is now outdated
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
   void homa_sendmsg(hls::stream<sendmsg_raw_t> & sendmsg_i,
         hls::stream<sendmsg_t> & sendmsg_o) {

#pragma HLS pipeline II=1
         sendmsg_raw_t sendmsg_raw = sendmsg_i.read();

         sendmsg_t sendmsg;

         sendmsg.buffin            = sendmsg_raw(SENDMSG_BUFFIN);
         sendmsg.length            = sendmsg_raw(SENDMSG_LENGTH);
         sendmsg.saddr             = sendmsg_raw(SENDMSG_SADDR);
         sendmsg.daddr             = sendmsg_raw(SENDMSG_DADDR);
         sendmsg.sport             = sendmsg_raw(SENDMSG_SPORT);
         sendmsg.dport             = sendmsg_raw(SENDMSG_DPORT);
         sendmsg.id                = sendmsg_raw(SENDMSG_ID);
         sendmsg.completion_cookie = sendmsg_raw(SENDMSG_CC);
         sendmsg.rtt_bytes         = sendmsg_raw(SENDMSG_RTT);
         sendmsg.granted           = (sendmsg_raw(SENDMSG_RTT) > sendmsg_raw(SENDMSG_LENGTH)) ? sendmsg_raw(SENDMSG_LENGTH) : sendmsg_raw(SENDMSG_RTT);

         sendmsg_o.write(sendmsg);
   }
}
