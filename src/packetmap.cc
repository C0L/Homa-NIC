#include "packetmap.hh"

/**
 * packetmap() - Determines when resent requests need to be issued and
 * manages packet bitmaps. Will evaluate touch requests (reset timer/update packetmap)
 * immediately, but crawls through RPCs to evaluate resend requests. Resend requests
 * contain the offset of the first packet that needs to be resent.
 * @touch:  Incoming stream from link_ingress. Resets the timer on an RPC and updates
 * packet maps.
 * @complete: RPCs that have an all '1' packetmap, and thus have been completely
 * received, have their RPC ID placed on the complete stream.
 * @rexmit: Outgoing stream to link_egress. RPC that needs a RESEND request.
 */
void packetmap(hls::stream<header_t> & header_in, 
      hls::stream<header_t> & complete_messages) {

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

   if (header_in.read_nb(header_in)) {

      packetmap_t packetmap;
      if (header_in.offset == 0) {
         packetmap.map = 0;
         packetmap.head = 0;
         // TODO need to convert to bit index?
         packetmap.length = (header_in.length / HOMA_PAYLOAD_SIZE);
      } else {
         packetmap = packetmaps[rexmit_touch.rpc_id];
      }

      // TODO need to convert to bit index
      int diff = (header_in.segment_offset / HOMA_PAYLOAD_SIZE) - packetmap.head;

      if (diff <= 0 || diff > 64) {
         continue;
      }

      packetmap.map[diff-1] = 1;

      int shift = 0;
      for (int i = 63; i >= 0; --i) {
         if (packetmap.map[i] == 0) shift = i;
      }

      packetmap.head += shift;
      packetmap.map <<= shift;

      // Has the head reached the length?
      if (packetmap.head - packetmap.length == 0) {
         // Notify recv system that the message is fully buffered
         complete_out.write(header_in.local_id);

         // Disable this RPC
         last_touches[header_in.locak_id] = 0;
      }

      packetmaps[header_in.local_id] = packetmap;
   }

   // TODO currently disabled timer componant

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
