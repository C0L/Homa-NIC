#include "timer.hh"
#include "srptmgmt.hh"

/**
 * rexmit_buffer() - Determines when RESEND requests need to be issued
 * @touch:  Incoming stream from link_ingress. Resets the timer on an RPC 
 * @rexmit: Outgoing stream to link_egress. RPC that needs a RESEND request.
 * Note: when an RPC has been completed, to prevent incorrect RESEND request
 * the RPC entry must be cleared. Instead of using an explicit "delete" stream
 * just touch with a value of 0xFFFFFFFFFFFFFFFF to the respective RPC, which
 * will effectively disable it.
 */
void rexmit_buffer(hls::stream<rexmit_t> & touch, 
		   hls::stream<rpc_id_t> & complete,
		   hls::stream<rexmit_t> & rexmit) {

  // If offset is -1, then just touch message and leave the packetmap alone?

  static uint64_t rexmit_store[MAX_RPCS];

  // TODO This is probably too expensive?
  static packetmap_t packetmaps[MAX_RPCS];
  
  static fifo_t<rpc_id_t, 2> refresh;
  
  // No dependencies within loops or between iterations
#pragma HLS dependence variable=rexmit_store inter WAR false
#pragma HLS dependence variable=rexmit_store inter RAW false
#pragma HLS dependence variable=rexmit_store intra WAR false
#pragma HLS dependence variable=rexmit_store intra RAW false
  
  static uint64_t time = 0;
  
  /*
   * This loop has an interation latency of 3 cycles but II of 1
   * The reason we need a small FIFO is that it is not possible to
   * 1) touch an RPC, 2) refresh the RPC value after a RESEND
   * in the same cycle due to BRAM pressure.
   * To resolve this we unify all insertions (RPC touches/RESEND updates)
   * via the FIFO so that only a single insertion takes place per iteration
   * A multi-ported RAM would resolve this
   */
  for (rpc_id_t i = 0; i < MAX_RPCS; ++i) {
    rexmit_t rexmit_touch;
  
    if (!refresh.empty()) {
      rpc_id_t rpc_id = refresh.remove();
      rexmit_store[rpc_id] = time;
    } else if (touch.read_nb(rexmit_touch)) {
      rexmit_store[rexmit_touch.rpc_id] = time;

      // A max offset of -1 indicates update timer only
      if (rexmit_touch.max_offset != -1) {
	packetmap_t packetmap = packetmaps[rexmit_touch.rpc_id];

	packetmap[rexmit_touch.offset] = 1;
	// Compare against -1 (all ones)
	if (packetmap.range(rexmit_touch.max_offset, 0).and_reduce()) {
	  complete.write(rexmit_touch.rpc_id);
	}

	packetmaps[rexmit_touch.rpc_id] = packetmap;
      }
    }
      
    uint64_t entry_time = rexmit_store[i];
      
    if (entry_time - time >= REXMIT_CUTOFF) {
      if (rexmit.write_nb({i,0,0})) {
	// TODO Need lower and upper bound for retransmission?
	refresh.insert(i);
      }
    }
  
    time++;
  }
}

