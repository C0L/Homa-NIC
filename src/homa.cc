#include <iostream>
#include <ap_int.h>
#include <hls_stream.h>
#include <ap_axi_sdata.h>

#include "hls_task.h"
#include "homa.hh"
#include "dma.hh"
//#include "link.hh"
#include "rpcmgmt.hh"
#include "srptmgmt.hh"
#include "xmitbuff.hh"

using namespace std;

//ap_uint<64> timer;

void temp(hls::stream<xmit_req_t> & xmit_buffer_request,
	  hls::stream<xmit_mblock_t> & xmit_buffer_response,
	  hls::stream<xmit_id_t> & xmit_stack_free,
	  hls::stream<homa_rpc_id_t> & rpc_stack_free,
	  hls::stream<homa_rpc_t> & rpc_table_insert,
	  hls::stream<srpt_entry_t> & srpt_queue_next) {
  // Garbage to pass compilation
  xmit_req_t xmit_req;
  xmit_buffer_request.write(xmit_req);

  xmit_mblock_t mblock;
  xmit_buffer_response.read(mblock);

  xmit_id_t xmit_id;
  xmit_stack_free.write(xmit_id);

  homa_rpc_id_t homa_rpc_id;
  rpc_stack_free.write(homa_rpc_id);

  homa_rpc_t homa_rpc;
  rpc_table_insert.write(homa_rpc);

  srpt_entry_t entry;
  srpt_queue_next.read(entry);
}


/**
 * homa() - Top level homa packet processor
 * @homa:
 * @link_ingress: The incoming AXI Stream of ethernet frames from the link
 * @link_egress:  The outgoing AXI Stream of ethernet frames from to the link
 * @dma:   DMA memory space pointer
 */
void homa(homa_t * homa,
	  params_t * params,
	  hls::stream<raw_frame_t> & link_ingress,
	  hls::stream<raw_frame_t> & link_egress,
	  hls::burst_maxi<xmit_mblock_t> maxi) {

  // TODO need correct control params for axi lite?

  // #pragma HLS INTERFACE ap_ctrl_none port=return TODO?
#pragma HLS INTERFACE s_axilite port=homa
#pragma HLS INTERFACE s_axilite port=params
#pragma HLS INTERFACE axis port=link_ingress
#pragma HLS INTERFACE axis port=link_egress
  //#pragma HLS INTERFACE axis port=sdma
#pragma HLS interface mode=m_axi port=maxi latency=100 num_read_outstanding=32 num_write_outstanding=32 max_read_burst_length=16 max_write_burst_length=16

  // https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/Tasks-and-Channels critical

  //#pragma HLS pipeline rewind TODO?
  // Dataflow required for m_axi https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/Tasks-and-Dataflow

  /* update_rpc_stack */
  hls_thread_local hls::stream<homa_rpc_id_t> rpc_stack_next;
  hls_thread_local hls::stream<homa_rpc_id_t> rpc_stack_free;
  
  /* update_rpc_table */
  hls_thread_local hls::stream<hashpack_t> rpc_table_request;
  hls_thread_local hls::stream<homa_rpc_id_t> rpc_table_response;
  hls_thread_local hls::stream<homa_rpc_t> rpc_table_insert;
  
  /* update_rpc_buffer */
  hls_thread_local hls::stream<homa_rpc_id_t> rpc_buffer_request_primary;
  hls_thread_local hls::stream<homa_rpc_t> rpc_buffer_response_primary;
  hls_thread_local hls::stream<homa_rpc_id_t> rpc_buffer_request_secondary;
  hls_thread_local hls::stream<homa_rpc_t> rpc_buffer_response_secondary;
  hls_thread_local hls::stream<homa_rpc_t> rpc_buffer_insert;

  /* update_xmit_buffer */
  hls_thread_local hls::stream<xmit_in_t> xmit_buffer_insert;
  hls_thread_local hls::stream<xmit_req_t> xmit_buffer_request;
  hls_thread_local hls::stream<xmit_mblock_t> xmit_buffer_response;

  /* update_xmit_stack */
  hls_thread_local hls::stream<xmit_id_t> xmit_stack_next;
  hls_thread_local hls::stream<xmit_id_t> xmit_stack_free;
  
  /* update_srpt_queue */
  hls_thread_local hls::stream<srpt_entry_t> srpt_queue_insert;
  hls_thread_local hls::stream<srpt_entry_t> srpt_queue_next;


#pragma HLS dataflow
  /* Control Driven Region */

  /* Pull user requests from DMA and coordinate ingestion */
  dma_ingress(homa,
	      params,
	      maxi,
	      xmit_buffer_insert,
	      xmit_stack_next,
	      rpc_stack_next,
	      rpc_table_request,
	      rpc_table_response,
	      rpc_buffer_request_secondary,
	      rpc_buffer_response_secondary,
	      rpc_buffer_insert,
	      srpt_queue_insert);

  /* Data Driven Region */

  /* Check for new messages that need to be onboarded and fill a message buffer if needed */
  hls_thread_local hls::task thread_1(update_xmit_buffer,
				      xmit_buffer_insert,
				      xmit_buffer_request,
				      xmit_buffer_response);

  hls_thread_local hls::task thread_2(update_xmit_stack,
				      xmit_stack_next,
				      xmit_stack_free);

  hls_thread_local hls::task thread_3(update_rpc_stack,
				      rpc_stack_next,
				      rpc_stack_free);

  /* Insert new RPCs and perform searches for RPCs */
  hls_thread_local hls::task thread_4(update_rpc_table,
				      rpc_table_request,
				      rpc_table_response,
				      rpc_table_insert);

  /* Insert new peers and perform lookup on the peer table */
  //hls_thread_local hls::task update_peer_table(, );



  /* Perform any needed operations on the stored RPC data */
  hls_thread_local hls::task thread_5(update_rpc_buffer,
				      rpc_buffer_request_primary,
				      rpc_buffer_response_primary,
				      rpc_buffer_request_secondary,
				      rpc_buffer_response_secondary,
				      rpc_buffer_insert);

  /* Perform any needed operations on the stored RPC data */
  //hls_thread_local hls::task update_peer_buffer(, .... );

  /* Update the SRPT queue */
  hls_thread_local hls::task thread_6(update_srpt_queue,
				      srpt_queue_insert,
				      srpt_queue_next);

  // TODO add a very simple link path that takes a message buffer worth of data per the SRPT queue rpc to test single cycle behavior
  // SET II=1 to make sure it can get a packet every cycle

     temp(xmit_buffer_request,
	  xmit_buffer_response,
	  xmit_stack_free,
	  rpc_stack_free,
	  rpc_table_insert,
	  srpt_queue_next);


  // TODO? Depth of srpt out should be a few elements

  //timer++;
}

