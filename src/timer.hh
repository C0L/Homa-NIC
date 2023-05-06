#ifndef TIMER_H
#define TIMER_H

#include "rpcmgmt.hh"

struct timer_touch_t {
  rpc_id_t rpc_id;
  ap_uint<64> update_time;
};

void update_timer(hls::stream<ap_uint<1>> & timer_request_0,
		  hls::stream<ap_uint<64>> & timer_response_0);

//void rexmit_buffer(hls::stream<rpc_id_t> & touch,
//		   hls::stream<rpc_id_t> & rexmit);

#endif
