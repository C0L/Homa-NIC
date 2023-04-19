#include <string.h>

#include "link.hh"

#include "srptmgmt.hh"
#include "rpcmgmt.hh"

/**
 * proc_link_egress() - 
 */
//void proc_link_egress(hls::stream<raw_frame_t> & link_egress) {
//
//#pragma HLS pipeline II=1
//
//  if (srpt_queue.size != 0) {
//  
//    homa_rpc_t & homa_rpc = rpcs[homa_rpc_id];
//
//    raw_frame_t outframe;
//
//    // TODO temporary
//    for (int i = 0; i < 1522; ++i) {
//#pragma HLS unroll
//      outframe.data[i] = homa_rpc.msgout.message[i];
//    }
//
//    // TODO Cannot use memcpy not from external interface?
//    //memcpy(&outframe, &homa_rpc.msgout.message, sizeof(raw_frame_t));
//
//    link_egress.write(outframe);
//  
//    // P4 style pipeline
//    //ingress_op1();
//    //ingress_op2();
//    //ingress_op3();
//  
//    //link_egress.write();
//  
//    //if (homa_rpc.homa_message_out.length != 0) {
//      // Recycle RPC ID?
//    //}
//  }
//}

///**
// * Parse the ethernet, ipv4, homa, header of the incoming frame
// * Check conditions (mac destination, ethertype, etc)
// */
//void parse_homa(raw_frame_t & frame) {
//  // Cast onto ethernet header
//  // Check mac destination address
//  // Check ethertype
//  // Cast onto ipv4 header
//  // Check version, destination, protocol
//  // Cast onto Homa header
//}


/**
 * proc_link_ingress() - 
 * 
 */
void proc_link_ingress(hls::stream<raw_frame_t> & link_ingress,
		       char * ddr_ram,
		       user_output_t * dma_egress) {
  // Complete the frame processing, P4 style
  #pragma HLS pipeline
  raw_frame_t frame;

  // Has a full ethernet frame been buffered? If so, grab it.
  if (link_ingress.full()) {
    link_ingress.read(frame);

    // Don't change control flow for a bad packet
    //bool discard = parse_homa(frame, );
  
    // This seems like the most obvious way to accomplish p4 behavior 
    //ingress_op1();
    //ingress_op2();
    //ingress_op3();
  }
}

