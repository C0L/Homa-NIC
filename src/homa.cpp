#include <iostream>
#include <ap_int.h>
#include <hls_stream.h>

 #include <ap_axi_sdata.h>
// TODO which one?
// #include "ap_axi_sdata.h"
#include "homa.hh"
#include "dma.hh"
#include "ingress.hh"
#include "egress.hh"

// AXI4-Stream is a protocol designed for transporting arbitrary unidirectional data. In an AXI4-Stream, TDATA width of bits is transferred per clock cycle. The transfer is started once the producer sends the TVALID signal and the consumer responds by sending the TREADY signal (once it has consumed the initial TDATA). At this point, the producer will start sending TDATA and TLAST (TUSER if needed to carry additional user-defined sideband data). TLAST signals the last byte of the stream. So the consumer keeps consuming the incoming TDATA until TLAST is asserted.

// https://discuss.pynq.io/t/tutorial-pynq-dma-part-1-hardware-design/3133
// https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/AXI4-Stream-Interfaces-without-Side-Channels
// https://support.xilinx.com/s/question/0D52E00006iHuY7SAK/axi-stream-interface-help?language=en_US
// https://docs.xilinx.com/r/en-US/pg085-axi4stream-infrastructure/TUSER-Width-bits
// https://github.com/Xilinx/HLS_packet_processing/blob/master/apps/arp/app.cpp
// https://docs.xilinx.com/r/2022.1-English/ug1393-vitis-application-acceleration/Free-Running-Kernel

/*
 * FAQ:
 *   1) What is hls::stream?
 * 
 *   2) When is the top level function "called"?
 *
 *   3) What is #pragma HLS INTERFACE ap_ctrl_none port=?
 *
 *   4) What is #pragma HLS INTERFACE axis port=?
 *
 *   5) What is #pragma HLS dataflow?
 *
 *   6) What is #pragma HLS pipeline?
 */

using namespace std;

/* Top-Level Homa Processing Function 
 *     TODO: this is a free running core
 *     You can read this as if it were sequential C code, but the #pragmas translate it when wsynthesized
 */
void homa(hls::stream<raw_frame>   & link_ingress,
	  hls::stream<raw_frame>   & link_egress,
	  hls::stream<request_in>  & dma_ingress,
	  hls::stream<request_out> & dma_egress ) {
// This makes it a free running kernel!
#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS INTERFACE axis port=link_ingress
#pragma HLS INTERFACE axis port=link_egress
#pragma HLS INTERFACE axis port=dma_ingress
#pragma HLS INTERFACE axis port=dma_egress

  // TODO partition these arrays
  /*
   * Storage for all active outgoing RPCs, which there are a maximum of MAX_RPCS of
   *   The static qualifier makes this persist across kernel/function invocations (think every cycle)
   *   An RPC contains.... TODO
   */
  static rpc[MAX_RPCS];
  static message_out[MAX_RPCS];
  static message_in[MAX_RPCS];
  static next_rpc = 0;

  /*
   * The following functions can be performed largely in parallel as indicated by DATAFLOW directive
   * thought its evaluation always always behave identically as if it were sequential
   *   proc_link_egress() : process incoming frames from the link
   *   proc_dma()         : process host machine requests (send, rec, reply) and return info back
   *   proc_link_ingress(): pop the RPC from the SRPT queue and send
   *   update_outgoing()  : determine what the next message to be broadcast will be
   *   TODO - update_outgoing will either just update outgoing_rpc, or update that and the srpt queue
   */
#pragma HLS dataflow 
  proc_link_egress(link_egress,);
  proc_dma(dma_ingress, dma_egress);
  proc_link_ingress(link_ingress);
//  update_outgoing();
}

