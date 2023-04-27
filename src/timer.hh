#ifndef TIMER_H
#define TIMER_H

ap_uint<64> timer;

void update_timer(hls::stream<ap_uint<1>> & timer_request_0,
		  hls::stream<ap_uint<64>> & timer_response_0) {
  timer++;

  ap_uint<1> request_0;
  if (timer_request_0.read_nb(request_0)) {
    timer_response_0.write_nb(timer);
  }
}

#endif
