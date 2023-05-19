#include <iostream>
#include <ap_int.h>

#include "ap_axi_sdata.h"
#include "hls_task.h"
#include "hls_stream.h"

#include "homa.hh"
#include "dma.hh"
#include "link.hh"
#include "rpcmgmt.hh"
#include "srptmgmt.hh"
#include "xmitbuff.hh"
#include "timer.hh"

using namespace std;

// This is just used to tie off some streams for compilation sake
void temp(hls::stream<xmit_id_t> & xmit_stack_free,
	  hls::stream<rpc_id_t> & rpc_stack_free,
	  hls::stream<peer_id_t> & peer_stack_free,
	  hls::stream<peer_id_t> & peer_buffer_request,
	  hls::stream<homa_peer_t> & peer_buffer_response,
	  hls::stream<rpc_id_t> & rexmit_complete) {
  // Garbage to pass compilation
  // This may interrupt normal behavior if it is hammering some path in the output stream
  xmit_id_t xmit_id;
  xmit_stack_free.write(xmit_id);

  rpc_id_t homa_rpc_id;
  rpc_stack_free.write(homa_rpc_id);

  peer_id_t homa_peer_id;
  peer_stack_free.write(homa_peer_id);

  peer_id_t req_id;
  peer_buffer_request.write(req_id);

  homa_peer_t resp_peer;
  peer_buffer_response.read(resp_peer);

  rpc_id_t rexmit;
  rexmit_complete.read(rexmit);
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
	  hls::burst_maxi<xmit_mblock_t> maxi_in, // TODO use same interface for both axi then remove bundle
	  xmit_mblock_t * maxi_out) {

#pragma HLS interface s_axilite port=homa bundle=config
#pragma HLS interface s_axilite port=params bundle=onboard

#pragma HLS interface axis port=link_ingress
#pragma HLS interface axis port=link_egress

#pragma HLS interface mode=m_axi port=maxi_in bundle=maxi_0 latency=60 num_read_outstanding=32 num_write_outstanding=32 max_read_burst_length=16 max_write_burst_length=16
#pragma HLS interface mode=m_axi port=maxi_out bundle=maxi_1 latency=60 num_read_outstanding=32 num_write_outstanding=32 max_read_burst_length=16 max_write_burst_length=16

  // https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/Tasks-and-Channels critical
  // Dataflow required for m_axi https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/Tasks-and-Dataflow

  /*
   * Tasks are effectively free-running threads that are infinitely looping
   * This more explicitly defines the parallelism of the processor
   * However, the way in which tasks interact is restricted—they can only comunicate via streams
   * As a result we are effectively turning every function call in the linux implementation into a stream interaction
   */

  /* update_rpc_stack */
  hls_thread_local hls::stream<rpc_id_t> rpc_stack_next_primary;
  hls_thread_local hls::stream<rpc_id_t> rpc_stack_next_secondary;
  hls_thread_local hls::stream<rpc_id_t> rpc_stack_free;
  
  /* update_rpc_table */
  hls_thread_local hls::stream<rpc_hashpack_t> rpc_table_request_primary;
  hls_thread_local hls::stream<rpc_id_t> rpc_table_response_primary;
  hls_thread_local hls::stream<rpc_hashpack_t> rpc_table_request_secondary;
  hls_thread_local hls::stream<rpc_id_t> rpc_table_response_secondary;
  hls_thread_local hls::stream<rpc_hashpack_t> rpc_table_request_ternary;
  hls_thread_local hls::stream<rpc_id_t> rpc_table_response_ternary;
  hls_thread_local hls::stream<homa_rpc_t> rpc_table_insert;
  
  /* update_rpc_buffer */
  hls_thread_local hls::stream<rpc_id_t> rpc_buffer_request_primary;
  hls_thread_local hls::stream<homa_rpc_t> rpc_buffer_response_primary;
  hls_thread_local hls::stream<rpc_id_t> rpc_buffer_request_secondary;
  hls_thread_local hls::stream<homa_rpc_t> rpc_buffer_response_secondary;
  hls_thread_local hls::stream<rpc_id_t> rpc_buffer_request_ternary;
  hls_thread_local hls::stream<homa_rpc_t> rpc_buffer_response_ternary;
  hls_thread_local hls::stream<homa_rpc_t> rpc_buffer_insert_primary;
  hls_thread_local hls::stream<homa_rpc_t> rpc_buffer_insert_secondary;

  /* update_xmit_buffer */
  hls_thread_local hls::stream<xmit_in_t> xmit_buffer_insert;
  hls_thread_local hls::stream<xmit_req_t> xmit_buffer_request;
  hls_thread_local hls::stream<xmit_mblock_t> xmit_buffer_response;

  /* update_xmit_stack */
  hls_thread_local hls::stream<xmit_id_t> xmit_stack_next;
  hls_thread_local hls::stream<xmit_id_t> xmit_stack_free;
  
  /* update_xmit_srpt_queue */
  hls_thread_local hls::stream<srpt_xmit_entry_t> srpt_xmit_queue_insert;
  hls_thread_local hls::stream<srpt_xmit_entry_t> srpt_xmit_queue_update;
  hls_thread_local hls::stream<srpt_xmit_entry_t> srpt_xmit_queue_next;

  /* update_grant_srpt_queue */
  hls_thread_local hls::stream<srpt_grant_entry_t> srpt_grant_queue_insert;
  hls_thread_local hls::stream<srpt_grant_entry_t> srpt_grant_queue_receipt;
  hls_thread_local hls::stream<srpt_grant_entry_t> srpt_grant_queue_next;

  /* dma_egress */
  hls_thread_local hls::stream<dma_egress_req_t> dma_egress_reqs;

  /* update_peer_stack */
  hls_thread_local hls::stream<peer_id_t> peer_stack_next_primary;
  hls_thread_local hls::stream<peer_id_t> peer_stack_next_secondary;
  hls_thread_local hls::stream<peer_id_t> peer_stack_free;
  
  /* update_peer_table */
  hls_thread_local hls::stream<peer_hashpack_t> peer_table_request_primary;
  hls_thread_local hls::stream<peer_id_t> peer_table_response_primary;
  hls_thread_local hls::stream<peer_hashpack_t> peer_table_request_secondary;
  hls_thread_local hls::stream<peer_id_t> peer_table_response_secondary;
  hls_thread_local hls::stream<homa_peer_t> peer_table_insert_primary;
  hls_thread_local hls::stream<homa_peer_t> peer_table_insert_secondary;
  
  /* update_peer_buffer */
  hls_thread_local hls::stream<peer_id_t> peer_buffer_request;
  hls_thread_local hls::stream<homa_peer_t> peer_buffer_response;
  hls_thread_local hls::stream<homa_peer_t> peer_buffer_insert_primary;
  hls_thread_local hls::stream<homa_peer_t> peer_buffer_insert_secondary;

  /* rexmit_buffer */
  hls_thread_local hls::stream<rexmit_t> rexmit_touch;
  hls_thread_local hls::stream<rpc_id_t> rexmit_complete;
  hls_thread_local hls::stream<rexmit_t> rexmit_rpcs;

#pragma HLS dataflow
  /* Control Driven Region */

  /* Pull user requests from DMA and coordinate ingestion */
  dma_ingress(homa,
	      params,
	      maxi_in,
	      xmit_buffer_insert,
	      xmit_stack_next,
	      rpc_stack_next_secondary,
	      rpc_table_request_secondary,
	      rpc_table_response_secondary,
	      rpc_buffer_request_ternary,
	      rpc_buffer_response_ternary,
	      rpc_buffer_insert_secondary,
	      srpt_xmit_queue_insert,
	      peer_stack_next_secondary,
	      peer_table_request_secondary,
	      peer_table_response_secondary,
	      peer_table_insert_secondary,
	      peer_buffer_insert_secondary);

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
				      rpc_stack_next_primary,
				      rpc_stack_next_secondary,
				      rpc_stack_free);

  /* Insert new RPCs and perform searches for RPCs */
  hls_thread_local hls::task thread_4(update_rpc_table,
				      rpc_table_request_primary,
				      rpc_table_response_primary,
				      rpc_table_request_secondary,
				      rpc_table_response_secondary,
				      rpc_table_insert);

  /* Perform any needed operations on the stored RPC data */
  hls_thread_local hls::task thread_5(update_rpc_buffer,
				      rpc_buffer_request_primary,
				      rpc_buffer_response_primary,
				      rpc_buffer_request_secondary,
				      rpc_buffer_response_secondary,
				      rpc_buffer_request_ternary,
				      rpc_buffer_response_ternary,
				      rpc_buffer_insert_primary,
				      rpc_buffer_insert_secondary);

  /* Update the SRPT queue */
  hls_thread_local hls::task thread_6(update_xmit_srpt_queue,
				      srpt_xmit_queue_insert,
				      srpt_xmit_queue_update,
				      srpt_xmit_queue_next);


  /* Update the SRPT queue */
  hls_thread_local hls::task thread_7(update_grant_srpt_queue,
				      srpt_grant_queue_insert,
				      srpt_grant_queue_receipt,
				      srpt_grant_queue_next);

  /* Send packets */
  hls_thread_local hls::task thread_8(proc_link_egress,
				      srpt_xmit_queue_next,
				      srpt_grant_queue_next,
				      xmit_buffer_request,
				      xmit_buffer_response,
				      rpc_buffer_request_secondary,
				      rpc_buffer_response_secondary,
				      rexmit_rpcs,
				      link_egress);

  hls_thread_local hls::task thread_9(proc_link_ingress,
				      link_ingress,
				      srpt_grant_queue_insert,
				      srpt_xmit_queue_update,
				      rpc_table_request_primary,
				      rpc_table_response_primary,
				      rpc_table_insert,
				      rpc_buffer_request_primary,
				      rpc_buffer_response_primary,
				      rpc_buffer_insert_primary,
				      rpc_stack_next_primary,
				      peer_table_request_primary,
				      peer_table_response_primary,
				      peer_buffer_insert_primary,
				      peer_table_insert_primary,
				      peer_stack_next_primary,
				      rexmit_touch,
				      dma_egress_reqs);


  hls_thread_local hls::task thread_11(update_peer_stack,
				       peer_stack_next_primary,
				       peer_stack_next_secondary,
				       peer_stack_free);

  hls_thread_local hls::task thread_12(update_peer_table,
				       peer_table_request_primary,
				       peer_table_response_primary,
				       peer_table_request_secondary,
				       peer_table_response_secondary,
				       peer_table_insert_primary,
				       peer_table_insert_secondary);

  hls_thread_local hls::task thread_13(update_peer_buffer,
				       peer_buffer_request,
				       peer_buffer_response,
				       peer_buffer_insert_primary,
				       peer_buffer_insert_secondary);

  hls_thread_local hls::task thread_14(rexmit_buffer,
				       rexmit_touch,
				       rexmit_complete,
				       rexmit_rpcs);
 
  dma_egress(dma_egress_reqs,
	     maxi_out);


  temp(xmit_stack_free,
       rpc_stack_free,
       peer_stack_free,
       peer_buffer_request,
       peer_buffer_response,
       rexmit_complete);
}

