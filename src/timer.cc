#include "timer.hh"


// TODO confirm this is actually free running
// May need to annotate this as a free running pipeline
void update_timer(hls::stream<ap_uint<1>> & timer_request_0,
		  hls::stream<ap_uint<64>> & timer_response_0) {

  static ap_uint<64> timer = 0;

  timer++;

  ap_uint<1> request_0;
  if (timer_request_0.read_nb(request_0)) {
    timer_response_0.write_nb(timer);
  }
}

//void rexmit_buffer(hls::stream<rpc_id_t> & touch,
//		   hls::stream<rpc_id_t> & rexmit) {
//  static timer_touch_t rexmit_times[MAX_RPCS];
//
//  // TODO get timer value
//  
//  for (rpc_id_t i = 0; i < MAX_RPCS; ++i) {
//    rpc_id_t rpc_id;
//    if (touch.read(rpc_id)) {
//      rexmit_times[rpc_id].update_time = time;
//    }
//
//    // TODO need to ensure timer is actually cycles
//    if (rexmit_times[i].update_time - current_time >= 1000000) {
//      rexmit.write(rexmit_times[i].rpc_id);
//    }
//  }
//}

