#include <string.h>

#include "homa.hh"
#include "link.hh"
#include "dma.hh"

/**
 * proc_link_egress() - 
 */
void proc_link_egress(hls::stream<srpt_xmit_entry_t> & srpt_xmit_queue_next,
		      hls::stream<srpt_grant_entry_t> & srpt_grant_queue_next,
		      hls::stream<xmit_req_t> & xmit_buffer_request,
		      hls::stream<xmit_mblock_t> & xmit_buffer_response,
		      hls::stream<rpc_id_t> & rpc_buffer_request,
		      hls::stream<homa_rpc_t> & rpc_buffer_response,
		      hls::stream<raw_frame_t> & link_egress) {
#pragma HLS PIPELINE II=1 
  
  srpt_xmit_entry_t srpt_xmit_entry;
  srpt_xmit_queue_next.read(srpt_xmit_entry);

  srpt_grant_entry_t srpt_grant_entry;
  srpt_grant_queue_next.write(srpt_grant_entry);

  homa_rpc_t homa_rpc;
  rpc_buffer_request.write(srpt_xmit_entry.rpc_id);
  rpc_buffer_response.write(homa_rpc);

  raw_frame_t raw_frame;
  // TODO this is temporary and wrong just to test datapath
  for (int i = 0; i < XMIT_BUFFER_SIZE; ++i) {
    xmit_mblock_t mblock;
    //xmit_req_t xmit_req = {homa_rpc.msgout.xmit_id, (xmit_offset_t) i};
    xmit_buffer_request.write((xmit_req_t) {homa_rpc.msgout.xmit_id, (xmit_offset_t) i});
    xmit_buffer_response.read(mblock);
    raw_frame.data[i%6] = mblock;
    if (i % 6 == 0) link_egress.write(raw_frame);
  }
  

  // Message buffer ID should be in homa_rpc
  // current block should also be stored
  // 
  //      while (unscheduled bytes != 0)...
  //  #pragma HLS unroll
  //        xmit_mblock_t mblock;
  //      xmit_buffer_request({current block})
  //        raw_frame_t raw_frame;
  //        TODO begin populating
}

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
// TODO will need to access DMA
void proc_link_ingress(hls::stream<raw_frame_t> & link_ingress,
		       hls::stream<srpt_grant_entry_t> & srpt_grant_queue_insert,
		       hls::stream<srpt_xmit_entry_t> & srpt_xmit_queue_update,
		       hls::stream<dma_egress_req_t> & dma_egress_reqs) {
#pragma HLS PIPELINE II=1 
  raw_frame_t frame;
#pragma HLS array_partition variable=frame type=complete

  srpt_grant_entry_t entry;
  srpt_grant_queue_insert.write(entry);

  srpt_xmit_entry_t xentry;
  srpt_xmit_queue_update.write(xentry);

  // Has a full ethernet frame been buffered? If so, grab it.
  link_ingress.read(frame);

  // TODO this needs to get the address of the output
  for (int i = 0; 6; ++i) {
    dma_egress_req_t req = {i, frame.data[i]};
    dma_egress_reqs.write(req);
  }
}

