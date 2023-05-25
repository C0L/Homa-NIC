#include "timer.hh"

/**
 * rexmit_buffer() - Determines when resent requests need to be issued and
 * manages packet bitmaps. Will evaluate touch requests (reset timer/update packetmap)
 * immediately, but crawls through RPCs to evaluate resend requests. Resend requests
 * contain the offset of the first packet that needs to be resent.
 * @touch:  Incoming stream from link_ingress. Resets the timer on an RPC and updates
 * packet maps.
 * @complete: RPCs that have an all '1' packetmap, and thus have been completely
 * received, have their RPC ID placed on the complete stream.
 * @rexmit: Outgoing stream to link_egress. RPC that needs a RESEND request.
 *
 * TODO: when an RPC has been completed, to prevent incorrect RESEND request
 * the RPC entry is cleared. However, if an RPC were to fail their needs to be
 * an explicit reset process.
 */
void rexmit_buffer(hls::stream<touch_t> & touch_in, 
		   hls::stream<rpc_id_t> & complete_out,
		   hls::stream<rexmit_t> & rexmit_out) {

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
  static uint64_t last_touches[MAX_RPCS];
#pragma HLS dependence variable=last_touches inter WAR false
#pragma HLS dependence variable=last_touches inter RAW false

  static packetmap_t packetmaps[MAX_RPCS];
#pragma HLS dependence variable=packetmaps inter WAR false
#pragma HLS dependence variable=packetmaps inter RAW false
#pragma HLS dependence variable=packetmaps intra WAR false
#pragma HLS dependence variable=packetmaps intra RAW false

  static uint64_t time = 0;

  // Take touch requests if we have them, otherwise crawl through RPCs
  for (rpc_id_t bram_id = 0; bram_id < MAX_RPCS;) {
    touch_t rexmit_touch;
    
    if (touch_in.read_nb(rexmit_touch)) {
      
      packetmap_t packetmap;
      if (rexmit_touch.init) {
	packetmap.map = 0;
	packetmap.head = 0;
	packetmap.length = rexmit_touch.length;
      } else {
	packetmap = packetmaps[rexmit_touch.rpc_id];
      }

      int diff = rexmit_touch.offset - packetmap.head;

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
	// Notify user msg is complete?
	complete_out.write(rexmit_touch.rpc_id);

	// Disable this RPC
	last_touches[rexmit_touch.rpc_id] = 0;
      } else {
	last_touches[rexmit_touch.rpc_id] = time;
      }

      packetmaps[rexmit_touch.rpc_id] = packetmap;
    } else {
      uint64_t last_touch = last_touches[bram_id];
            
      if (last_touch != 0 && time - last_touch >= REXMIT_CUTOFF) {
      	// Where should retransmission begin?
      	packetmap_t packetmap = packetmaps[bram_id];
            
	if (rexmit_out.write_nb({bram_id, packetmap.head+1})) {
	  last_touches[bram_id] = time;
	}
      }

      bram_id++;
    }
  
    time++;
  }
}


//void rexmit_buffer(hls::stream<rexmit_t> & touch, 
//		   hls::stream<rpc_id_t> & complete,
//		   hls::stream<rexmit_t> & rexmit) {
//
//  /* 
//   * A 64 bit timestamp is stored for each RPC. 
//   * If the difference (+MAX_RPCS cycles) between the value stored
//   * in the rexmit_store, and the current time, is over 200000 then
//   * trigger a resend.
//   *
//   * We need to specify that multiple loop iterations will not have a
//   * memory dependence (i.e. memory access across loop iterations
//   * to rexmit_store are not overlapping). "inter" WAR/RAW specify
//   * we do not consider these true memory dependencies.
//   */
//  static uint64_t rexmit_store[MAX_RPCS];
//#pragma HLS dependence variable=rexmit_store inter WAR false
//#pragma HLS dependence variable=rexmit_store inter RAW false
//
//  /*
//   * Currently,
//   *   block size PM_BS = 32,
//   *   num blocks PM_NB = 32,
//   *
//   * Packetmaps is a 2^14 * 32 array of 32 bit values. This choice was made based
//   * on the difficulty of mapping a 1D array or a more appropriate 2D array properly
//   * with a cyclic BRAM mapping. 32 bit values fit well given the max output port
//   * size of BRAMs is 36.
//   *
//   * Cyclic partitioning is needed because we need to pull an entire 1024 bitmap out
//   * for evaluating completness in a single cycle, so elements of the bitmap must be
//   * aggregated.
//   *
//   * We specify that there are no memory dependencies across or within loop iterations.
//   */
//  static ap_uint<PM_BS> packetmaps[MAX_RPCS * PM_NB];
//#pragma HLS array_partition type=cyclic factor=PM_NB variable=packetmaps
//#pragma HLS dependence variable=packetmaps inter WAR false
//#pragma HLS dependence variable=packetmaps inter RAW false
//#pragma HLS dependence variable=packetmaps intra WAR false
//#pragma HLS dependence variable=packetmaps intra RAW false
//  
//  static uint64_t time = 0;
//
//  // Take touch requests if we have them, otherwise crawl through RPCs
//  for (rpc_id_t bram_id = 0; bram_id < MAX_RPCS;) {
//    rexmit_t rexmit_touch;
//  
//    if (touch.read_nb(rexmit_touch)) {
//
//      ap_uint<PM_BS> packetmap[PM_NB];
//#pragma HLS array_partition variable=packetmap complete
//
//      if (rexmit_touch.init) {
//
//	int block = rexmit_touch.length / PM_BS;
//	int offset = rexmit_touch.length - (block * PM_BS);
//
//	for (int i = 0; i < PM_NB; ++i) {
//#pragma HLS unroll
//	  if (i < block) packetmap[i] = 0; else if (i > block) packetmap[i] = ~((ap_uint<PM_BS>) 0);
//	}
//
//	ap_uint<PM_BS> transition = ~((ap_uint<PM_BS>) 0);
//	packetmap[block] = transition >> offset;
//      } else {
//	for (int i = 0; i < PM_NB; ++i) {
//#pragma HLS unroll
//	  packetmap[i] = packetmaps[rexmit_touch.rpc_id * PM_BS + i];
//	}
//      }
//
//      int block = rexmit_touch.offset / PM_BS;
//      int offset = rexmit_touch.offset - (block * PM_BS);
//     
//      packetmap[block][offset] = 1;
//
//      // Compare against all ones
//      bool done = true;
//      for (int i = 0; i < PM_NB; ++i) {
//#pragma HLS unroll
//	if (packetmap[i] != ~((ap_uint<PM_BS>) 0)) done = false;
//      }
//
//      if (done) {
//	// Notify user msg is complete?
//	complete.write(rexmit_touch.rpc_id);
//
//	// Disable this RPC
//	rexmit_store[rexmit_touch.rpc_id] = 0;
//      } else {
//	rexmit_store[rexmit_touch.rpc_id] = time;
//      }
//
//      for (int i = 0; i < PM_NB; ++i) {
//#pragma HLS unroll
//	packetmaps[rexmit_touch.rpc_id * PM_BS + i] = packetmap[i];
//      }
//    } else {
//      uint64_t entry_time = rexmit_store[bram_id];
//            
//      if (entry_time != 0 && time - entry_time >= REXMIT_CUTOFF) {
//      	// Where should retransmission begin?
//	ap_uint<PM_BS> block;
//
//	int blockoffset = 0;
//      	for (int i = PM_NB-1; i >= 0; --i) {
//#pragma HLS unroll
//      	  if (packetmaps[bram_id * PM_BS + i] != ~((ap_uint<PM_BS>) 0)) {
//	    block = packetmaps[bram_id * PM_BS + i];
//	    blockoffset = i;
//	  }
//      	}
//
//	int bitoffset = 0;
//	for (int i = PM_NB-1; i >= 0; --i) {
//#pragma HLS unroll
//	  if (block[i] == 0) bitoffset = i;
//	}
//            
//      	if (rexmit.write_nb({bram_id, false, (blockoffset * PM_BS + bitoffset), 0})) {
//      	  rexmit_store[bram_id] = time;
//      	}
//      }
//
//      bram_id++;
//    }
//  
//    time++;
//  }
//}
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//void rexmit_buffer(hls::stream<rexmit_t> & touch_in, 
//		   hls::stream<rpc_id_t> & complete_out,
//		   hls::stream<rexmit_t> & rexmit_out) {
//
//  /* 
//   * A 64 bit timestamp is stored for each RPC. 
//   * If the difference (+MAX_RPCS cycles) between the value stored
//   * in the rexmit_store, and the current time, is over 200000 then
//   * trigger a resend.
//   *
//   * We need to specify that multiple loop iterations will not have a
//   * memory dependence (i.e. memory access across loop iterations
//   * to rexmit_store are not overlapping). "inter" WAR/RAW specify
//   * we do not consider these true memory dependencies.
//   */
//  static uint64_t rexmit_store[MAX_RPCS];
//#pragma HLS dependence variable=rexmit_store inter WAR false
//#pragma HLS dependence variable=rexmit_store inter RAW false
//
//   /*
//    * Currently,
//    *   block size PM_BS = 32,
//    *   num blocks PM_NB = 32,
//    *
//    * Packetmaps is a 2^14 * 32 array of 32 bit values. This choice was made based
//    * on the difficulty of mapping a 1D array or a more appropriate 2D array properly
//    * with a cyclic BRAM mapping. 32 bit values fit well given the max output port
//    * size of BRAMs is 36.
//    *
//    * Cyclic partitioning is needed because we need to pull an entire 1024 bitmap out
//    * for evaluating completness in a single cycle, so elements of the bitmap must be
//    * aggregated.
//    *
//    * We specify that there are no memory dependencies across or within loop iterations.
//    */
//  static packetmap_t packetmaps[MAX_RPCS];
//  //#pragma HLS array_partition type=block factor=PM_NB variable=packetmaps dim=2
//#pragma HLS dependence variable=packetmaps inter WAR false
//#pragma HLS dependence variable=packetmaps inter RAW false
//#pragma HLS dependence variable=packetmaps intra WAR false
//#pragma HLS dependence variable=packetmaps intra RAW false
//
//    static uint64_t time = 0;
//
//    // Take touch requests if we have them, otherwise crawl through RPCs
//    for (rpc_id_t bram_id = 0; bram_id < MAX_RPCS;) {
//      rexmit_t rexmit_touch;
//
//      if (touch.read_nb(rexmit_touch)) {
//
//	packetmap_t packetmap;
//	if (rexmit_touch.init) {
//	  packetmap.map = 0;
//	  packetmap.head = 0
//	} else {
//	  packetmap = packetmaps[rexmit_touch.rpd_id];
//	}
//
//	packetmap[rexmit_touch.offset - head] = 1;
//
//	// Compare against all ones
//	bool done = true;
// 
//	//     for (int i = 0; i < PM_NB * PM_BS; ++i) {
//	//#pragma HLS unroll
//	//	if (packetmap[i] != 1) done = false;
//	//	//if (packetmap[i] != ~((ap_uint<PM_BS>) 0)) done = false;
//	//      }
//
//	// Compare against -1
//	if (!packetmap.and_reduce()) {
//	  done = false;
//	}
//
//      if (done) {
//	// Notify user msg is complete?
//	complete.write(rexmit_touch.rpc_id);
//
//	// Disable this RPC
//	rexmit_store[rexmit_touch.rpc_id] = 0;
//      } else {
//	rexmit_store[rexmit_touch.rpc_id] = time;
//      }
//
//      for (int i = 0; i < PM_NB*PM_BS; ++i) {
//#pragma HLS unroll
//	packetmaps[rexmit_touch.rpc_id][i] = packetmap[i];
//      }
//    } else {
//    //      uint64_t entry_time = rexmit_store[bram_id];
//    //            
//    //      if (entry_time != 0 && entry_time - time >= REXMIT_CUTOFF) {
//    //      	// Where should retransmission begin?
//    //	ap_uint<PM_BS> block;
//    //
//    //	int blockoffset = 0;
//    //      	for (int i = PM_NB-1; i >= 0; --i) {
//    //#pragma HLS unroll
//    //      	  if (packetmaps[bram_id * PM_BS + i] != ~((ap_uint<PM_BS>) 0)) {
//    //	    block = packetmaps[bram_id * PM_BS + i];
//    //	    blockoffset = i;
//    //	  }
//    //      	}
//    //
//    //	int bitoffset = 0;
//    //	for (int i = PM_NB-1; i >= 0; --i) {
//    //#pragma HLS unroll
//    //	  if (block[i] == 0) bitoffset = i;
//    //	}
//    //            
//	  if (rexmit.write_nb({bram_id, false, 0, 0})) {
//    //if (rexmit.write_nb({bram_id, false, (blockoffset * PM_BS + bitoffset), 0})) {
//      rexmit_store[bram_id] = time;
//    }
//    //      }
//
//      ++bram_id;
//    }
//  
//    time++;
//  }
//}
