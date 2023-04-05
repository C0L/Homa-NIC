#include <iostream>
#include <ap_int.h>
#include <hls_stream.h>
#include <ap_axi_sdata.h>

#include "homa.hh"
#include "dma.hh"
#include "link.hh"

using namespace std;

/**
 * FAQ:
 *   1) What is hls::stream?
 *     A C interface to the AXI Stream protocol. AXI Stream is an on chip transport protocol for efficiently moving data. From: https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/How-AXI4-Stream-Works, "AXI4-Stream is a protocol designed for transporting arbitrary unidirectional data. In an AXI4-Stream, TDATA width of bits is transferred per clock cycle. The transfer is started once the producer sends the TVALID signal and the consumer responds by sending the TREADY signal (once it has consumed the initial TDATA). At this point, the producer will start sending TDATA and TLAST (TUSER if needed to carry additional user-defined sideband data). TLAST signals the last byte of the stream. So the consumer keeps consuming the incoming TDATA until TLAST is asserted."
 *
 *     hls::stream abstracts the AXI Stream protocol to give a simple interface for checking state (full, empty), reading from streams, or writing to streams. More information here: https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/HLS-Stream-Library.
 * 
 *   2) When is the top level function "called"?
 *     This kernel is set into "free running mode", meaning that the instructions of each pipeline stage will be execute each clock cycle (ideally at 200MHz).
 *
 *     Refer to: https://docs.xilinx.com/r/2022.1-English/ug1393-vitis-application-acceleration/Free-Running-Kernel
 *
 *   3) What is #pragma HLS INTERFACE ap_ctrl_none port=return?
 *     It is not always desirable to run kernels in free running mode, and sometimes other ctrl ports are desired. We are not interested in this for a continuous packet processor. 
 *
 *   4) What is #pragma HLS INTERFACE axis port=XXXXX?
 *     This decares the port as an AXI Stream interface. 
 *
 *   5) What is #pragma HLS DATAFLOW?
 *     Though the execution in hardware of this kernel will always behave the same as that of the impertive code, ideally there are oportunities for parallelization. One type of parallelization is at the task-level, identified by the DATAFLOW directive, which indicates there are oportunities for running a set of functions as seperate parallel tasks. Though there may be some dependence between them, the compiler will search for most overlapping executing of the functions identified by the data flow directive. For example, processing input ethernet frames (parsing the packet, ip, homa) is relatively independent of outgoing frames (pop from SRPT and send). Similar independence is found with the user DMA user transaction.
 * 
 *     More information can be found on the directive here: https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/pragma-HLS-dataflow
 *
 *   6) What is #pragma HLS PIPELINE?
 *
 *   7) What is #pragma HLS INTERFACE m_axi ...?
 */

// TODO add debug mode with print output 

//https://docs.xilinx.com/v/u/en-US/wp454-ultrascale-memory

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
  static homa_rpc_t rpcs[MAX_RPCS];
  static rpc_stack_t rpc_stack;
  static srpt_queue_t srpt_queue;

  /**
   * The following functions can be performed largely in parallel as indicated by DATAFLOW directive
   * though its evaluation will always behave identically as if it were sequential. If there are
   * dependencies across these functions then the tool will be unable to hit the 5ns period.
   *   proc_link_egress() : process incoming frames from the link
   *   proc_link_ingress(): pop the RPC from the SRPT queue and send
   *   proc_dma_ingress() : process host machine requests (send, rec, reply) and return info back
   */
#pragma HLS dataflow
  proc_link_egress(link_egress, rpcs, rpc_stack, srpt_queue);
  proc_link_ingress(link_ingress, rpcs, ddr_ram, rpc_stack, dma_egress);
  proc_dma_ingress(dma_ingress, rpcs, ddr_ram, dma_egress, rpc_stack, srpt_queue);
}

