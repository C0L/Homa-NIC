#include <iostream>
#include <ap_int.h>
#include <hls_stream.h>
#include <ap_axi_sdata.h>

#include "homa.hh"
#include "dma.hh"
#include "link.hh"
#include "rpcmgmt.hh"

using namespace std;

ap_uint<64> timer;

// TODO: It may be wise to split proc_dma_ingress into an individual kernel do that the dataflow directive does not cause packet processing to stall. This could also maybe be solved by running the link ingress/egress multiple times in one execution of the homa core. Will need to look at the schedule viewer.

// TODO: May need pipeline rewind?
// TODO: https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/Controlling-Hardware
// TODO: Vars need to be made static to persist across kernel invocations (rpcs, srpt...)

/**
 * homa() - Top level homa packet processor
 * @homa:
 * @link_ingress: The incoming AXI Stream of ethernet frames from the link
 * @link_egress:  The outgoing AXI Stream of ethernet frames from to the link
 * @dma:   DMA memory space pointer
 */
void homa(homa_t * homa, 
	  hls::stream<raw_frame_t> & link_ingress,
	  hls::stream<raw_frame_t> & link_egress,
	  hls::stream<args_t> & sdma,
	  hls::burst_maxi<ap_uint<512>> mdma) {
	  //char * mdma) {
	  //ap_uint<512> * mdma) {
// This makes it a free running kernel

  //#pragma HLS INTERFACE ap_ctrl_none port=return

#pragma HLS INTERFACE s_axilite port=homa
#pragma HLS INTERFACE axis port=link_ingress
#pragma HLS INTERFACE axis port=link_egress
#pragma HLS INTERFACE axis port=sdma
#pragma HLS interface mode=m_axi port=mdma latency=100 num_read_outstanding=32 num_write_outstanding=32 max_read_burst_length=16 max_write_burst_length=16

  //max_widen_bitwidth=512 alignment_byte_size 512


  //static srpt_queue_t srpt_queue;

  // https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/Controlling-Hardware

  // TODO need rewind directive for continuous execution across kernel invocations


  // TODO need some FIFO like structure for SRPT and RPC stack
  // Only need to read or write once per cycle. Otherwise data just lives in FIFO once more
  // Would decouple this nicely

  // while (data on input dma && data on srpt && data on ingress && data on egress ) 

  //while (!user_req.empty() && srpt_queue.size != 0 && !link_ingress.empty() && !link_egress.empty()) {
  //#pragma HLS pipeline rewind
  //#pragma HLS dataflow
  //#pragma HLS INTERFACE ap_ctrl_none port=return

    /**
     * The following functions can be performed ltargely in parallel as indicated by DATAFLOW directive
     * though its evaluation will always behave identically as if it were sequential. If there are
     * dependencies across these functions then the tool will be unable to hit the 5ns period.
     *   proc_link_egress() : process incoming frames from the link
     *   proc_link_ingress(): pop the RPC from the SRPT queue and send
     *   proc_dma_ingress() : process host machine requests (send, rec, reply) and return info back
     */

    // Probably easier to just do/while on a var indicating the state of DMA ingress
    // SRPT queue should only be accessed once in link egress. Should be local to egress. It should have a stream input that tells new srpts, that can be addded to the queue when availible. How to handle RPCs and peers though?

  // Default depth of 2, use STREAM directive to modify
  //hls::stream<homa_rpc_id_t> popstream, pushstream;

    //update_state(popstream, pushstream);
    //proc_link_egress(link_egress, popstream);
    //proc_link_ingress(link_ingress, dma);
    dma_ingress(homa, sdma, mdma);

    // This should be incremented once each time a packet completes processing?
    //timer++;
    //}
}

// TODO need to pass remaining bytes around as well to avoid RPC Lookup
//void update_state(hls::stream<homa_rpc_id_t> popstream, hls::stream<homa_rpc_id_t> pushstream) {
//  // Do we need to saturate the outgoing SRPT path or process new SRPTs to add
//  if (!popstream.full()) {
//    homa_rpc_id_t homa_rpc_id = srpt_queue.pop();
//    popstream.write(homa_rpc_id);
//  } else if (!pushstream.empty()) {
//    homa_rpc_id_t homa_rpc_id;
//    pushstream.read(homa_rpc_id);
//    srpt_queue.push(homa_rpc_id);
//  }
//}

