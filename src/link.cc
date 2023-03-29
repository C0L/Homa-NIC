#include "homa.hh"
#include "ingress.hh"

/* TODO what does this function accomplish
 * Process the arrival of a raw ethernet frame of max size 12176 bits
 */
void proc_link_ingress(hls::stream<raw_frame> & link_ingress,
		       rpc[MAX_RPCS]          & rpcs) {
  // Complete the frame processing, P4 style
  #pragma HLS pipeline
  raw_frame frame;
  // Has a full ethernet frame been buffered? If so, grab it.
  if (link_ingress.full()) {
    link_ingress.read(frame);
  }

  // Don't change control flow for a bad packet
  //bool discard = parse_homa(frame, );
  
  // This seems like the most obvious way to accomplish p4 behavior 
  //ingress_op1();
  //ingress_op2();
  //ingress_op3();
}

/*
 * Parse the ethernet, ipv4, homa, header of the incoming frame
 * Check conditions (mac destination, ethertype, etc)
 */
void parse_homa(raw_frame & frame) {
  // Cast onto ethernet header
  // Check mac destination address
  // Check ethertype
  // Cast onto ipv4 header
  // Check version, destination, protocol
  // Cast onto Homa header
}


#include "homa.hh"

/* 
 *
 */
void proc_link_egress(hls::stream<raw_frame> & link_egress,
		      homa_rpc (&rpcs)[MAX_RPCS],
		      srpt_queue_t & srpt_queue) {
  static int remaining = 0;
  #pragma HLS pipeline

  // TODO are we actively sending something??

  //ingress_op1();
  //ingress_op2();
  //ingress_op3();
 
  if (link.egress.empty()) {
    link_egress.write();
  }
}


