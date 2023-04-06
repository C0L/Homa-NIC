#include <iostream>
#include <ap_int.h>
#include <hls_stream.h>
#include <ap_axi_sdata.h>

#include "homa.hh"
#include "dma.hh"
#include "link.hh"

using namespace std;

static ap_uint<64> timer; 

/**
 * homa() - Top level homa packet processor 
 * @link_ingress: The incoming AXI Stream of ethernet frames from the link
 * @link_egress:  The outgoing AXI Stream of ethernet frames from to the link
 * @ddr_ram:      On PCB RAM pointer
 * @dma_egress:   DMA memory space pointer
 */
void homa(hls::stream<raw_frame_t> & link_ingress,
	  hls::stream<raw_frame_t> & link_egress,
	  hls::stream<user_input_t> & dma_ingress,
	  char * ddr_ram,
	  user_output_t * dma_egress) {
// This makes it a free running kernel
#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS INTERFACE axis port=link_ingress
#pragma HLS INTERFACE axis port=link_egress
#pragma HLS INTERFACE axis port=dma_ingress
// TODO configure number of unreturned requests?
#pragma HLS INTERFACE m_axi depth=1 port=ddr_ram
#pragma HLS INTERFACE m_axi depth=1 port=dma_egress

  /**
   * Storage for all active outgoing RPCs, which there are a maximum of MAX_RPCS of
   *   The static qualifier makes this persist across kernel/function invocations (think every cycle)
   */

  //static srpt_queue_t srpt_queue;

  /**
   * The following functions can be performed largely in parallel as indicated by DATAFLOW directive
   * though its evaluation will always behave identically as if it were sequential. If there are
   * dependencies across these functions then the tool will be unable to hit the 5ns period.
   *   proc_link_egress() : process incoming frames from the link
   *   proc_link_ingress(): pop the RPC from the SRPT queue and send
   *   proc_dma_ingress() : process host machine requests (send, rec, reply) and return info back
   */
#pragma HLS dataflow

  proc_link_egress(link_egress);
  proc_link_ingress(link_ingress, ddr_ram, dma_egress);
  proc_dma_ingress(dma_ingress, ddr_ram, dma_egress);

  timer++;
}

