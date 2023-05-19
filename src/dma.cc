#include <string.h>

#include "ap_int.h"
#include "dma.hh"
#include "rpcmgmt.hh"
#include "srptmgmt.hh"

// TODO If the message is short < 1000 bytes, it should bypass DMA and bypass the SRPT system
/**
 * dma_ingress()
 * Pull user request from DMA and the first message caches worth of bytes
 */
void dma_ingress(homa_t * homa,
		 params_t * params,
		 hls::burst_maxi<xmit_mblock_t> maxi,
		 hls::stream<xmit_in_t> & xmit_buffer_insert,
		 hls::stream<xmit_id_t> & xmit_stack_next,
		 hls::stream<rpc_id_t> & rpc_stack_next,
		 hls::stream<rpc_hashpack_t> & rpc_table_request,
		 hls::stream<rpc_id_t> & rpc_table_response,
		 hls::stream<rpc_id_t> & rpc_buffer_request,
		 hls::stream<homa_rpc_t> & rpc_buffer_response,
		 hls::stream<homa_rpc_t> & rpc_buffer_insert,
		 hls::stream<srpt_xmit_entry_t> & srpt_queue_insert,
		 hls::stream<peer_id_t> & peer_stack_next,
		 hls::stream<peer_hashpack_t> & peer_table_request,
		 hls::stream<peer_id_t> & peer_table_response,
		 hls::stream<homa_peer_t> & peer_table_insert,
		 hls::stream<homa_peer_t> & peer_buffer_insert) {
  // TODO this function signature is absurd. Pack into structs maybe?
#pragma HLS interface mode=ap_ctrl_hs port=return
  
  xmit_id_t xmit_id;
  xmit_stack_next.read(xmit_id);

  // AXI burst request
  maxi.read_request(params->buffin, XMIT_BUFFER_SIZE);

  // Iteratively read result of burst and set to xmit buffer
  for (int i = 0; i < XMIT_BUFFER_SIZE; i++) {
#pragma HLS pipeline II=1
    xmit_in_t xmit_in = {maxi.read(), xmit_id, (xmit_offset_t) i};
    xmit_buffer_insert.write(xmit_in);
  }

  homa_rpc_t homa_rpc;

  // Initial RPC state configuration depending on request vs response
  if (!params->id) {
    /* This is a request message */

    // Grab an unused RPC ID (this is a client/even ID be default)
    rpc_stack_next.read(homa_rpc.rpc_id);

    homa_rpc.completion_cookie = params->completion_cookie;

    /* homa_rpc_new_client */
    //homa_rpc.id = rec_id;
    homa_rpc.state = homa_rpc_t::RPC_OUTGOING;
    homa_rpc.flags = 0;
    homa_rpc.grants_in_progress = 0;

    /* Equivalent to homa_peer_find */
    peer_hashpack_t hashpack = {params->dest_addr.sin6_addr.s6_addr};
    peer_table_request.write(hashpack); 
    peer_table_response.read(homa_rpc.peer_id);

    // If the peer table request returned 0, then no peer is found 
    if (homa_rpc.peer_id == 0) {
      // Get an available peer ID (an index into the peer buffer)
      peer_stack_next.read(homa_rpc.peer_id);

      // Create a new peer to add into the peer buffer
      homa_peer_t peer;
      peer.peer_id = homa_rpc.peer_id;
      peer.addr = params->dest_addr.sin6_addr.s6_addr;
      peer.unsched_cutoffs[HOMA_MAX_PRIORITIES-1] = 0;
      peer.unsched_cutoffs[HOMA_MAX_PRIORITIES-2] = INT_MAX;
      peer.cutoff_version = 0;
      peer.last_update_jiffies = 0;
      peer.outstanding_resends = 0;
      peer.most_recent_resend = 0;
      peer.least_recent_rpc = NULL;
      peer.least_recent_ticks = 0;
      peer.current_ticks = -1;
      peer.resend_rpc = NULL;
      peer.num_acks = 0;

      // Store the peer data
      peer_buffer_insert.write(peer);

      // Store the mapping from IP addres to peer
      peer_table_insert.write(peer);
    } 

    // TODO assume user formated port correctly? (removed the ntphs function)
    homa_rpc.addr = params->dest_addr.sin6_addr.s6_addr;
    homa_rpc.dport = params->dest_addr.sin6_port;
    homa_rpc.msgin.total_length = -1;
    homa_rpc.msgout.length = -1;
    homa_rpc.silent_ticks = 0;

    // TODO homa timer is unimplemented
    //homa_rpc.resend_timer_ticks = homa->timer_ticks;
    homa_rpc.done_timer_ticks = 0;
    homa_rpc.buffout = params->buffout;
    homa_rpc.buffin = params->buffin;
  } else {
    /* This is a response message */

    // TODO we are assuming input address is always IPv6 and little endian?
    rpc_id_t homa_rpc_id;

    /* homa_find_server_rpc */

    // Get local ID from address, ID, and port
    rpc_hashpack_t hashpack = {params->dest_addr.sin6_addr.s6_addr, params->id, params->dest_addr.sin6_port, 0};
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

  // TODO this can just be built into this core? Indicate we want a timer value 
  //timer_request.write(1);
  // Read the timer value 
  //timer_response.read(homa_rpc.msgout.init_cycles);

  // TODO don't compute this every time
  int max_pkt_data = homa->mtu - IPV6_HEADER_LENGTH - sizeof(data_header_t);

  // TODO all of this information may just be stored in its SRPT entry?
  if (homa_rpc.msgout.length <= max_pkt_data) { // TODO potentially elim this? rtt_bytes > max_pkt_data
    homa_rpc.msgout.unscheduled = homa_rpc.msgout.length;
  } else {
    homa_rpc.msgout.unscheduled = homa->rtt_bytes;
    if (homa_rpc.msgout.unscheduled > homa_rpc.msgout.length)
      homa_rpc.msgout.unscheduled = homa_rpc.msgout.length;
  }

  // An RPC is automatically granted the unscheduled number of bytes
  homa_rpc.msgout.granted = homa_rpc.msgout.unscheduled;

  // Update the homa_rpc in storage
  rpc_buffer_insert.write(homa_rpc);

  // Adds this RPC to the queue for broadcast
  srpt_xmit_entry_t srpt_entry = srpt_xmit_entry_t(homa_rpc.rpc_id, (uint32_t) homa_rpc.msgout.length, homa_rpc.msgout.unscheduled,  homa_rpc.msgout.length);

    //{homa_rpc.rpc_id, (uint32_t) homa_rpc.msgout.length};

  // This is sender side SRPT which is based on number of remaining bytes to transmit
  srpt_queue_insert.write(srpt_entry);
}

void dma_egress(hls::stream<dma_egress_req_t> & dma_egress_reqs,
		xmit_mblock_t * maxi_out) {
  // TODO this is a auto-restarting kernel
#pragma HLS INTERFACE mode=ap_ctrl_none port=return

  dma_egress_req_t dma_egress_req;
  dma_egress_reqs.read(dma_egress_req);

  *(maxi_out + dma_egress_req.offset) = dma_egress_req.mblock;
}
