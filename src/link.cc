#include "link.hh"

/**
 * proc_link_egress() - Pop an RPC from the SRPT queue, package its message
 * contents into packets, and place them onto the link.
 * @link_egress: The outgoing AXI Stream that will send packets onto the net
 * @rpcs:        RPC store
 * @rpc_stack:   RPCs that are not in use 
 * @srpt_queue:  Sorted queue of RPCs ready to broadcast based on smallest 
 *               number of packets remaining
 */
void proc_link_egress(hls::stream<raw_frame_t> & link_egress) {

  static bool broadcast_active = false;
  //static homa_rpc_id_t broadcast_homa_rpc_id;

  // TODO grant process currently unimplemented
  
#pragma HLS pipeline

  #ifdef DEBUG
  std::cerr << "DEBUG: Selecting Message for Egress:" << std::endl;
  std::cerr << "DEBUG:     -> Broadcast Active = " << broadcast_active << std::endl;
  //std::cerr << "DEBUG:     -> SRPT Queue Size = " << srpt_queue.size << std::endl;
  #endif

  //  if (!broadcast_active && srpt_queue.size != 0) {
  //    broadcast_active = true;
  //    broadcast_homa_rpc_id = srpt_queue.pop();
  //
  //    #ifdef DEBUG
  //    std::cerr << "DEBUG: Popped New Message for Broadcast:" << std::endl;
  //    std::cerr << "DEBUG:     -> Active RPC ID = " << broadcast_homa_rpc_id << std::endl;
  //    #endif
  //  }
  //  
  //  if (broadcast_active) {
  //    homa_rpc_t homa_rpc = rpcs[broadcast_homa_rpc]; 
  //
  //    // P4 style pipeline
  //    //ingress_op1();
  //    //ingress_op2();
  //    //ingress_op3();
  //
  //    //link_egress.write();
  //
  //    if (homa_rpc.homa_message_out.length != 0) {
  //      // Recycle RPC ID?
  //    }
  //  }
}

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
 * proc_link_ingress() - Processes incoming ethernet frames, writes the result to user
 * memory, and updates RPC state.
 * 
 * @link_ingress: The incoming AXI Stream of ethernet frames from the link
 * @rpcs:         RPC store
 * @ddr_ram:      On PCB RAM pointer
 * @rpc_stack:    RPCs that are not in use 
 * @dma_egress:   DMA memory space pointer
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

