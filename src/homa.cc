#include <iostream>
#include <ap_int.h>
#include <hls_stream.h>
#include <ap_axi_sdata.h>

#include "hls_task.h"
#include "homa.hh"
#include "dma.hh"
#include "link.hh"
#include "rpcmgmt.hh"
#include "srptmgmt.hh"

using namespace std;

//ap_uint<64> timer;

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
	  hls::burst_maxi<xmit_unit_t> mdma) {

  // #pragma HLS INTERFACE ap_ctrl_none port=return TODO?
#pragma HLS INTERFACE s_axilite port=homa
#pragma HLS INTERFACE axis port=link_ingress
#pragma HLS INTERFACE axis port=link_egress
#pragma HLS INTERFACE axis port=sdma
#pragma HLS interface mode=m_axi port=mdma latency=100 num_read_outstanding=32 num_write_outstanding=32 max_read_burst_length=16 max_write_burst_length=16

  // https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/Tasks-and-Channels critical

  //#pragma HLS pipeline rewind TODO?
  // Dataflow required for m_axi https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/Tasks-and-Dataflow
#pragma HLS dataflow
  /* update_rpc_stack */
  hls_thread_local hls::stream<homa_rpc_id_t> rpc_stack_next;
  hls_thread_local hls::stream<homa_rpc_id_t> rpc_stack_free;

  /* update_rpc_table */
  hls_thread_local hls::stream<hashpack_t> rpc_table_request;
  hls_thread_local hls::stream<homa_rpc_id_t> rpc_table_response;
  hls_thread_local hls::stream<homa_rpc_t> rpc_table_insert;

  /* update_rpc_buffer */
  hls_thread_local hls::stream<homa_rpc_id_t> rpc_buffer_request;
  hls_thread_local hls::stream<homa_rpc_t> rpc_buffer_response;
  hls_thread_local hls::stream<homa_rpc_t> rpc_buffer_insert;

  /* update_xmit_buffer */
  hls_thread_local hls::stream<xmit_in_t> xmit_buffer_insert;
  hls_thread_local hls::stream<xmit_req_t> xmit_buffer_request;
  hls_thread_local hls::stream<xmit_out_t> xmit_buffer_response;

  /* update_xmit_stack */
  hls_thread_local hls::stream<xmit_id_t> xmit_stack_next;
  hls_thread_local hls::stream<xmit_id_t> xmit_stack_free;

  /* update_srpt_queue */
  hls_thread_local hls::stream<srpt_entry_t> srpt_queue_insert;
  hls_thread_local hls::stream<srpt_entry_t> srpt_queue_next;

  /* Pull user requests from DMA and coordinate ingestion */
  hls_thread_local hls::task thread_0(dma_ingress,
				      sdma,
				      xmit_buffer_insert,
				      xmit_stack_next,
				      rpc_stack_next,
				      rpc_table_request,
				      rpc_table_response,
				      rpc_buffer_request,
				      rpc_buffer_response,
				      rpc_buffer_insert,
				      srpt_queue_insert);

  /* Check for new messages that need to be onboarded and fill a message buffer if needed */
  hls_thread_local hls::task update_xmit_buffer(xmit_buffer_insert,
						xmit_buffer_request,
						xmit_buffer_response);

  hls_thread_local hls::task update_xmit_stack(xmit_stack_next,
					       xmit_stack_free);

  hls_thread_local hls::task update_rpc_stack(rpc_stack_next,
					      rpc_stack_free);

  /* Insert new RPCs and perform searches for RPCs */
  hls_thread_local hls::task update_rpc_table(rpc_table_request,
					      rpc_table_response,
					      rpc_table_insert);

  /* Insert new peers and perform lookup on the peer table */
  //hls_thread_local hls::task update_peer_table(, );



  /* Perform any needed operations on the stored RPC data */
  hls_thread_local hls::task update_rpc_buffer(rpc_buffer_request,
					       rpc_buffer_response,
					       rpc_buffer_insert);

  /* Perform any needed operations on the stored RPC data */
  //hls_thread_local hls::task update_peer_buffer(, .... );

  /* Update the SRPT queue */
  hls_thread_local hls::task update_srpt_queue(srpt_queue_insert,
					       srpt_queue_next);

  // TODO? Depth of srpt out should be a few elements

  //timer++;
}
