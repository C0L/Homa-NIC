#include "packetmap.hh"

/**
 * packetmap() - Determines when RPCs packet maps are complete and can be
 * returned to the user 
 * @header_in_i     - Incoming DATA packet headers which will update the state of
 * the packet map and potentially complete the RPC
 * @complete_msgs_o - When a packet map is complete, the header that triggered
 * that completion is forwarded back to the recv system so that the RPC can be
 * mactched with a recv call and the user notified
 */
void packetmap(hls::stream<header_t> & header_in_i, 
      hls::stream<header_t> & complete_msgs_o) {

   /* 
    * A 64 bit timestamp is stored for each RPC. 
    * If the difference (+MAX_RPCS cycles) between the value stored
    * in the rexmit_store, and the current time, is over 200000 then
    * trigger a resend.
    *
    * We need to specify that multiple loop iterations will not have a
    * memory dependence (i.e. memory access across loop iterations
    * to rexmit_store are not overlapping). "inter" WAR/RAW specify
    * we do not consider these true memory dependencies.
    */
//   static uint64_t last_touches[MAX_RPCS];
//#pragma HLS dependence variable=last_touches inter WAR false
//#pragma HLS dependence variable=last_touches inter RAW false

   static packetmap_t packetmaps[MAX_RPCS];
#pragma HLS dependence variable=packetmaps inter WAR false
#pragma HLS dependence variable=packetmaps inter RAW false
#pragma HLS dependence variable=packetmaps intra WAR false
#pragma HLS dependence variable=packetmaps intra RAW false

   header_t header_in;

   if (header_in_i.read_nb(header_in)) {
      packetmap_t packetmap;
      if (header_in.data_offset == 0) {
         packetmap.map = 0;
         packetmap.head = 0;
         packetmap.length = (header_in.message_length / HOMA_PAYLOAD_SIZE);
      } else {
	 packetmap = packetmaps[RECV_INDEX_FROM_RPC_ID(header_in.local_id)];
      }

      // TODO need to convert to bit index
      int diff = (header_in.data_offset / HOMA_PAYLOAD_SIZE) - packetmap.head;
         
      // Is this packet in bounds
      if (diff < 64) {
         packetmap.map[63-diff] = 1;

         // TODO should this be pipelined?
         int shift = 0;
         for (int i = 63; i >= 0; --i) {
            if (packetmap.map[63-i] == 0) shift = i;
         }

         packetmap.head += shift;
         packetmap.map <<= shift;

         // Has the head reached the length?
         if (packetmap.head - packetmap.length == 0) {
            // Notify recv system that the message is fully buffered
            complete_msgs_o.write(header_in);

            // Disable this RPC
            //last_touches[header_in.local_id] = 0;
         }

         packetmaps[RECV_INDEX_FROM_RPC_ID(header_in.local_id)] = packetmap;
	 std::cerr << "PACKET MAP OUT WITH ID " << header_in.id << std::endl;
      }
   }

   //static uint64_t time = 0;

   //// Take touch requests if we have them, otherwise crawl through RPCs
   //for (rpc_id_t bram_id = 0; bram_id < MAX_RPCS;) {
   //   touch_t rexmit_touch;

   //   if (touch_in.read_nb(rexmit_touch)) {

   //      packetmap_t packetmap;
   //      if (rexmit_touch.init) {
   //         packetmap.map = 0;
   //         packetmap.head = 0;
   //         packetmap.length = rexmit_touch.length;
   //      } else {
   //         packetmap = packetmaps[rexmit_touch.rpc_id];
   //      }

   //      int diff = rexmit_touch.offset - packetmap.head;

   //      if (diff <= 0 || diff > 64) {
   //         continue;
   //      }

   //      packetmap.map[diff-1] = 1;

   //      int shift = 0;
   //      for (int i = 63; i >= 0; --i) {
   //         if (packetmap.map[i] == 0) shift = i;
   //      }

   //      packetmap.head += shift;
   //      packetmap.map <<= shift;

   //      // Has the head reached the length?
   //      if (packetmap.head - packetmap.length == 0) {
   //         // Notify user msg is complete?
   //         complete_out.write(rexmit_touch.rpc_id);

   //         // Disable this RPC
   //         last_touches[rexmit_touch.rpc_id] = 0;
   //      } else {
   //         last_touches[rexmit_touch.rpc_id] = time;
   //      }

   //      packetmaps[rexmit_touch.rpc_id] = packetmap;
   //   } else {
   //      uint64_t last_touch = last_touches[bram_id];

   //      if (last_touch != 0 && time - last_touch >= REXMIT_CUTOFF) {
   //         // Where should retransmission begin?
   //         packetmap_t packetmap = packetmaps[bram_id];

   //         if (rexmit_out.write_nb({bram_id, packetmap.head+1})) {
   //            last_touches[bram_id] = time;
   //         }
   //      }

   //      bram_id++;
   //   }

   //   time++;
   //}
}
