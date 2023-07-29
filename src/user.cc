#include "user.hh"

extern "C" {

   /**
    * homa_recvmsg() - Maps completed messages to user level homa receive calls. 
    * @msghdr_recv_i - Incoming recvmsg requests from the user
    * @msghdr_recv_o - Returned recvmsg responsed with DMA offset
    * @header_in_i   - The final header received for a completed message. This
    * indicates the message has been fully received and buffering is complete.
    * TODO Should we for some notification that the message has been fully
    * buffered? If so the user could get the return recvmsg before the data has
    * made it to DMA?
    */
   void homa_recvmsg(hls::stream<msghdr_recv_t> & msghdr_recv_i,
         hls::stream<msghdr_recv_t> & msghdr_recv_o,
         hls::stream<header_t> & header_in_i) {

      // TODO this should be in BRAM, so cost is minimal
      static fifo_t<msghdr_recv_t,128> rec_response;
      static fifo_t<msghdr_recv_t,128> rec_request;

      // TODO do we need to return the shortest message first??
      // TODO instead of storing full header we can just store this
      static fifo_t<msghdr_recv_t,128> inc_response;
      static fifo_t<msghdr_recv_t,128> inc_request;

#pragma HLS pipeline II=1
      if (!msghdr_recv_i.empty()) {
         msghdr_recv_t msghdr_recv = msghdr_recv_i.read();

         // TODO pop from the respective inc fifo if the recv fifo is empty
         // Otherwise block
         
      } else if (!header_in_i) {
         header_in_i header_in = header_in_i.read();

         // TODO pop from the respective fifo, or block if both are empty

      } else {


      }
   }

   /**
    * sendmsg() - Injests new RPCs 
    * @msghdr_send_i - Homa configuration parameters
    * @msghdr_send_o - New RPCs that need to be onboarded
    */
   void homa_sendmsg(hls::stream<msghdr_send_t> & msghdr_send_i,
         hls::stream<msghdr_send_t> & msghdr_send_o,
         hls::stream<onboard_send_t> & onboard_send_o) {

#pragma HLS pipeline II=1

         msghdr_send_t msghdr_send = msghdr_send_i.read();

         onboard_send_t onboard_send;
         onboard_send.saddr      = msghdr_send(MSGHDR_SADDR);
         onboard_send.daddr      = msghdr_send(MSGHDR_DADDR);
         onboard_send.sport      = msghdr_send(MSGHDR_SPORT);
         onboard_send.dport      = msghdr_send(MSGHDR_DPORT);
         onboard_send.iov        = msghdr_send(MSGHDR_IOV);
         onboard_send.iov_size   = msghdr_send(MSGHDR_IOV_SIZE);
         onboard_send.id         = msghdr_send(MSGHDR_SEND_ID);
         onboard_send.cc         = msghdr_send(MSGHDR_SEND_CC);
         onboard_send.flags      = msghdr_send(MSGHDR_SEND_FLAGS);

         sendmsg_o.write(sendmsg);
   }
}
