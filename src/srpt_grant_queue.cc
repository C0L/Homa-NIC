#include "srpt_grant_queue.hh"

void srpt_grant_queue(hls::stream<ap_uint<125>> & in, hls::stream<ap_uint<1>> & out) {
  ap_uint<125> i = in.read();
  out.write(i);
}

//void srpt_grant_queue(ap_uint<14> peer_id_i, ap_uint<14> rpc_id_i, ap_uint<32> grantable_i, ap_uint<3> priority_i,
//		      ap_uint<1> ops_i, ap_uint<2> control_i, ap_uint<3> qs_i, ap_uint<14> & peer_id_o,
//		      ap_uint<14> & rpc_id_o, ap_uint<32> & grantable_o, ap_uint<3> & priority_o) {
//}
