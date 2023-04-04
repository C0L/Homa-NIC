#include "link.hh"

/* TODO what does this function accomplish
 * Process the arrival of a raw ethernet frame of max size 12176 bits
 */
void proc_link_ingress(hls::stream<raw_frame_t> & link_ingress,
		       homa_rpc_t * rpcs,
		       char * ddr_ram,
		       rpc_stack_t & send_ready,
		       user_output_t * dma_egress) {
  // Complete the frame processing, P4 style
  #pragma HLS pipeline
  raw_frame_t frame;
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
void parse_homa(raw_frame_t & frame) {
  // Cast onto ethernet header
  // Check mac destination address
  // Check ethertype
  // Cast onto ipv4 header
  // Check version, destination, protocol
  // Cast onto Homa header
}


/* 
 *
 */
void proc_link_egress(hls::stream<raw_frame_t> & link_egress,
		      homa_rpc_t * rpcs,
		      rpc_stack_t & rpc_stack,
		      srpt_queue_t & srpt_queue) {
  static int remaining = 0;
  #pragma HLS pipeline

  // TODO are we actively sending something??

  //ingress_op1();
  //ingress_op2();
  //ingress_op3();
 
  //if (link_egress.empty()) {
    //link_egress.write();
  //}
}


