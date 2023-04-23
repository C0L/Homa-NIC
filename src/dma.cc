#include <string.h>

#include "ap_int.h"
#include "dma.hh"
#include "rpcmgmt.hh"
#include "peer.hh"
#include "srptmgmt.hh"


// TODO If this message is very short we can just send directly?

/**
 * proc_dma_ingress()
 * @homa:
 * @user_req:
 * @dma:
 * Pull user request from DMA and the first message caches worth of bytes
 */
//void dma_ingress(hls::stream<args_t> & sdma,
//		 hls::stream<xmit_in_t> & xmit_buffer_insert,
//		 hls::stream<xmit_id_t> & xmit_stack_next,
//		 hls::stream<homa_rpc_id_t> & rpc_stack_next,
//		 hls::stream<hashpack_t> & rpc_table_request,
//		 hls::stream<homa_rpc_id_t> & rpc_table_response,
//		 hls::stream<homa_rpc_id_t> & rpc_buffer_request,
//		 hls::stream<homa_rpc_t> & rpc_buffer_response,
//		 hls::stream<homa_rpc_t> & rpc_buffer_insert,
//		 hls::stream<srpt_entry_t> & srpt_queue_insert) {

void dma_ingress(homa_t * homa,
		 params_t * params,
		 hls::burst_maxi<xmit_mblock_t> maxi,
		 hls::stream<xmit_in_t> & xmit_buffer_insert,
		 hls::stream<xmit_id_t> & xmit_stack_next,
		 hls::stream<homa_rpc_id_t> & rpc_stack_next,
		 hls::stream<hashpack_t> & rpc_table_request,
		 hls::stream<homa_rpc_id_t> & rpc_table_response,
		 hls::stream<homa_rpc_id_t> & rpc_buffer_request,
		 hls::stream<homa_rpc_t> & rpc_buffer_response,
		 hls::stream<homa_rpc_t> & rpc_buffer_insert,
		 hls::stream<srpt_entry_t> & srpt_queue_insert) {

  xmit_id_t xmit_id;
  xmit_stack_next.read(xmit_id);

  maxi.read_request(params->buffin, XMIT_BUFFER_SIZE);
  for (int i = 0; i < XMIT_BUFFER_SIZE; i++) {
#pragma HLS PIPELINE II=1
    xmit_in_t xmit_in = {maxi.read(), xmit_id, (xmit_offset_t) i};
    xmit_buffer_insert.write(xmit_in);
    //xmit_in.write(mdma.read());
      //tmp_buffer.second[i] = mdma.read();
  }

  //  for (int i = 0; i < XMIT_BUFFER_SIZE; ++i) {
  //#pragma HLS pipeline
  //    xmit_in_t xmit_in = {tmp_args.buffer.buff[i], xmit_id, (xmit_offset_t) i};
  //    xmit_buffer_insert.write(xmit_in);
  //  }

  homa_rpc_t homa_rpc;

  // Initial RPC state configuration depending on request vs response
  if (!params->id) {
    /* This is a request message */

    // Grab an unused RPC ID
    rpc_stack_next.read(homa_rpc.id);

    homa_rpc.completion_cookie = params->completion_cookie;

    /* homa_rpc_new_client */
    //homa_rpc.id = rec_id;
    homa_rpc.state = homa_rpc_t::RPC_OUTGOING;
    homa_rpc.flags = 0;
    homa_rpc.grants_in_progress = 0;

    // Locate associated peer ID TODO
    //search_peer.write(args.params.dest_addr);
    //serach_peer.read(homa_rpc.peer);

    // TODO removed the ntphs function? 
    homa_rpc.dport = params->dest_addr.sin6_port;
    homa_rpc.msgin.total_length = -1;
    homa_rpc.msgout.length = -1;
    homa_rpc.silent_ticks = 0;
    // TODO need timer core?
    //homa_rpc.resend_timer_ticks = homa->timer_ticks;
    homa_rpc.done_timer_ticks = 0;
    homa_rpc.buffout = params->buffout;
    homa_rpc.buffin = params->buffin;
  } else {
    // TODO we are assuming input address is always IPv6 and little endian?
    homa_rpc_id_t homa_rpc_id;

    /* homa_find_server_rpc */
    // Get local ID from address, ID, and port
    hashpack_t hashpack = {params->dest_addr.sin6_addr.s6_addr, params->id, params->dest_addr.sin6_port, 0};
    rpc_table_request.write(hashpack);
    rpc_table_response.read(homa_rpc_id);

    // Get actual RPC data from the local ID
    rpc_buffer_request.write(homa_rpc_id);
    rpc_buffer_response.read(homa_rpc);

    homa_rpc.state = homa_rpc_t::RPC_OUTGOING;
  }

  /* Outgoing message initialization */
  homa_rpc.msgout.length = params->length;
  homa_rpc.msgout.next_xmit_offset = 0;
  homa_rpc.msgout.sched_priority = 0;

  // TODO Need timer again?
  //homa_rpc.msgout.init_cycles = timer;
  
  //int max_pkt_data = homa.mtu - IPV6_HEADER_LENGTH - sizeof(data_header_t);
  // TODO need homa.mtu?
  int max_pkt_data = IPV6_HEADER_LENGTH - sizeof(data_header_t);
  
  // Does the message fit in a single packet?
  if (homa_rpc.msgout.length <= max_pkt_data) {
    homa_rpc.msgout.unscheduled = homa_rpc.msgout.length;
  } else {
    //homa_rpc.msgout.unscheduled = homa.rtt_bytes;
    // TODO need homa top level input?
    homa_rpc.msgout.unscheduled = 10000;
    if (homa_rpc.msgout.unscheduled > homa_rpc.msgout.length)
      homa_rpc.msgout.unscheduled = homa_rpc.msgout.length;
  }

  // An RPC is automatically granted the unscheduled number of bytes
  homa_rpc.msgout.granted = homa_rpc.msgout.unscheduled;

  // Update the homa_rpc in storage
  rpc_buffer_insert.write(homa_rpc);

  // Adds this RPC to the queue for broadcast
  srpt_entry_t srpt_entry = {homa_rpc.id, homa_rpc.msgout.unscheduled};
  srpt_queue_insert.write(srpt_entry);
}

//  // Have there been any user requests?
//  if (sdma.read_nb(tmp_args)) {
//    mdma.read_request(args.buffin, XMIT_BUFFER_SIZE);
//    for (int i = 0; i < XMIT_BUFFER_SIZE; i++) {
//#pragma HLS PIPELINE II=1
//      // TODO does this work?
//      xmit_in.write(mdma.read());
//      //tmp_buffer.second[i] = mdma.read();
//    }
//  }

