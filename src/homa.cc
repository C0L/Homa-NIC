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

/**
 * homa() - Top level homa packet processor 
 * @link_ingress: The incoming AXI Stream of ethernet frames from the link
 * @link_egress:  The outgoing AXI Stream of ethernet frames from to the link
 * @ddr_ram:      On PCB RAM pointer
 * @dma_egress:   DMA memory space pointer
 */
void homa(homa_t * homa, 
	  hls::stream<raw_frame_t> & link_ingress,
	  hls::stream<raw_frame_t> & link_egress,
	  hls::stream<dma_in_t> & user_req,
	  ap_uint<512> * dma) {
// This makes it a free running kernel
#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS INTERFACE s_axilite port=homa
#pragma HLS INTERFACE axis port=link_ingress
#pragma HLS INTERFACE axis port=link_egress
#pragma HLS INTERFACE axis port=user_req

  // Ensure transactions are bursted: https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/AXI-Burst-Transfers

#pragma HLS interface mode=m_axi port=dma latency=100 num_read_outstanding=32 num_write_outstanding=32 max_read_burst_length=16 max_write_burst_length=16 max_widen_bitwidth=1024
  //#pragma HLS interface mode=m_axi port=dma num_read_outstanding=32 num_write_outstanding=32 max_read_burst_length=16 max_write_burst_length=16

  // TODO readd latency dir?

  // TODO need to partititon arrays? look at access patterns

  /**
   * The following functions can be performed largely in parallel as indicated by DATAFLOW directive
   * though its evaluation will always behave identically as if it were sequential. If there are
   * dependencies across these functions then the tool will be unable to hit the 5ns period.
   *   proc_link_egress() : process incoming frames from the link
   *   proc_link_ingress(): pop the RPC from the SRPT queue and send
   *   proc_dma_ingress() : process host machine requests (send, rec, reply) and return info back
   */
//#pragma HLS dataflow

//proc_link_egress(link_egress);
//proc_link_ingress(link_ingress, ddr_ram, dma_egress);
  proc_dma_ingress(homa, user_req, dma);

  // This should be incremented once each time a packet completes processing?
  timer++;
}

