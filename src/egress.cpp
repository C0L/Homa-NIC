#include "homa.hh"

/* TODO what does this function accomplish
 *
 */
void proc_egress(hls::stream<raw_frame> & link_egress,
		 homa_rpc (&rpcs)[MAX_RPCS]) {
  #pragma HLS pipeline

  ingress_op1();
  ingress_op2();
  ingress_op3();
 
  if (link.egress.empty()) {
    link_egress.write();
  }
}
