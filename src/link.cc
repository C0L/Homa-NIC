#include <string.h>

#include "hls_math.h"

#include "homa.hh"
#include "link.hh"
#include "dma.hh"

/**
 * proc_link_egress() - 
 */
void proc_link_egress(hls::stream<srpt_xmit_entry_t> & srpt_xmit_queue_next,
		      hls::stream<srpt_grant_entry_t> & srpt_grant_queue_next,
		      hls::stream<xmit_req_t> & xmit_buffer_request,
		      hls::stream<xmit_mblock_t> & xmit_buffer_response,
		      hls::stream<rpc_id_t> & rpc_buffer_request,
		      hls::stream<homa_rpc_t> & rpc_buffer_response,
		      hls::stream<rexmit_t> & rexmit_rpcs,
		      hls::stream<raw_frame_t> & link_egress) {
#pragma HLS pipeline II=1
 
  srpt_xmit_entry_t srpt_xmit_entry;
  srpt_xmit_queue_next.read(srpt_xmit_entry);

  srpt_grant_entry_t srpt_grant_entry;
  srpt_grant_queue_next.write(srpt_grant_entry);

  rexmit_t rexmit;
  rexmit_rpcs.read(rexmit);

  homa_rpc_t homa_rpc;
  rpc_buffer_request.write(srpt_xmit_entry.rpc_id);
  rpc_buffer_response.read(homa_rpc);

  raw_frame_t raw_frame;
  // TODO this is temporary and wrong just to test datapath
  for (int i = 0; i < XMIT_BUFFER_SIZE; ++i) {
    xmit_mblock_t mblock;
    //xmit_req_t xmit_req = {homa_rpc.msgout.xmit_id, (xmit_offset_t) i};
    xmit_buffer_request.write((xmit_req_t) {homa_rpc.msgout.xmit_id, (xmit_offset_t) i});
    xmit_buffer_response.read(mblock);
    raw_frame.data[i%6] = mblock;
    if (i % 6 == 0) link_egress.write(raw_frame);
  }

}

/**
 * proc_link_ingress() - 
 * 
 */
void proc_link_ingress(hls::stream<raw_frame_t> & link_ingress,
		       hls::stream<srpt_grant_entry_t> & srpt_grant_queue_insert,
		       hls::stream<srpt_xmit_entry_t> & srpt_xmit_queue_update,
		       hls::stream<rpc_hashpack_t> & rpc_table_request,
		       hls::stream<rpc_id_t> & rpc_table_response,
		       hls::stream<homa_rpc_t> & rpc_table_insert,
		       hls::stream<rpc_id_t> & rpc_buffer_request,   		       
		       hls::stream<homa_rpc_t> & rpc_buffer_response,
		       hls::stream<homa_rpc_t> & rpc_buffer_insert,
		       hls::stream<rpc_id_t> & rpc_stack_next,
		       hls::stream<peer_hashpack_t> & peer_table_request,
		       hls::stream<peer_id_t> & peer_table_response,
		       hls::stream<homa_peer_t> & peer_buffer_insert,
		       hls::stream<homa_peer_t> & peer_table_insert,
		       hls::stream<peer_id_t> & peer_stack_next,
		       hls::stream<rexmit_t> & rexmit_touch,
		       hls::stream<dma_egress_req_t> & dma_egress_reqs) {

#pragma HLS pipeline II=1 

  /*
   * Are there incoming ethernet frames to process
   * Raw frame max size for non jumbo frames:
   *   6 + 6 + 4 + 2 + 1500 + 4 = 1522 bytes 
   */
  if (!link_ingress.empty()) {
    raw_frame_t raw_frame;
    link_ingress.read(raw_frame);

    ethernet_header_t * ethheader = (ethernet_header_t*) &(raw_frame.data);

    if (ethheader->ethertype != ETHERTYPE_IPV6) return;

    ipv6_header_t * ipv6header = (ipv6_header_t*) (((char*) ethheader) + sizeof(ethernet_header_t));

    // Version is a constant that should always be 6? 
    if (ipv6header->version != 0x6) return;

    // TODO traffic class? Nothing to do here?

    // TODO flow label? Nothing to do here?

    int payload_length = ipv6header->payload_length;

    if (ipv6header->next_header != IPPROTO_HOMA) return;

    // TODO Hop limit? Nothing to do here?

    // TODO Dest address? Nothing to do here?

    common_header_t * commonheader = (common_header_t *) (((char*) ipv6header) + sizeof(ipv6_header_t));

    /* homa_incoming.c — homa_pkt_dispatch() */

    uint64_t id = LOCALIZE_ID(commonheader->sender_id);

    // TODO there could be an ACK in the packet? check homa_incoming.c

    // A meaningful value in the rpc_buffer
    rpc_id_t local_id;
    homa_rpc_t homa_rpc;
   
    if (!IS_CLIENT(id)) {
      /* homa_find_server_rpc */

      // Search tuple for local rpc data from server rpc
      rpc_hashpack_t rpc_hashpack = {
	ipv6header->src_address,
	id,
	commonheader->sport,
	0
      };

      // Search for associated RPC ID
      rpc_table_request.write(rpc_hashpack);
      rpc_table_response.read(local_id);

      if (commonheader->doff == DATA) {
	/* homa_rpc_new_server */

	// Did the search fail? If so create a nw rpc
	if (local_id == 0) { 
	  homa_rpc.state = homa_rpc_t::RPC_INCOMING;

	  // TODO set flags/grants in progress?

	  /* homa_peer_find */

	  /* Equivalent to homa_peer_find */
	  peer_hashpack_t peer_hashpack = {ipv6header->src_address};
	  peer_table_request.write(peer_hashpack); 
	  peer_table_response.read(homa_rpc.peer_id);
	  
	  // If the peer table request returned 0, then no peer is found 
	  if (homa_rpc.peer_id == 0) {
	    // Get an available peer ID (an index into the peer buffer)
	    peer_stack_next.read(homa_rpc.peer_id);
	    
	    // Create a new peer to add into the peer buffer
	    homa_peer_t peer;
	    peer.peer_id = homa_rpc.peer_id;
	    peer.addr = ipv6header->src_address;
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

	  homa_rpc.addr = ipv6header->src_address;
	  homa_rpc.dport = commonheader->sport;
	  homa_rpc.completion_cookie = 0; // TODO irrelevent for server side
	  homa_rpc.msgin.total_length = -1;

	  // TODO homa_rpc.msgout?

	  homa_rpc.msgout.length = -1;
	  homa_rpc.silent_ticks = 0;
	  // TODO homa_rpc.resend_timer_ticks = homa->timer_ticks;
	  homa_rpc.done_timer_ticks = 0;

	  // Get a new local ID
	  rpc_stack_next.read(local_id);
	  homa_rpc.rpc_id = local_id;

	  rpc_buffer_insert.write(homa_rpc);

	  // Create a mapping from src address + id + sport -> local id
	  rpc_table_insert.write(homa_rpc);
	} else {
	  rpc_buffer_request.write(local_id);
	  rpc_buffer_response.read(homa_rpc);
	}
      }
    } else {
      // Client RPCs are meaningful in the rpc_buffer
      local_id = id;
    }

    switch(commonheader->doff) {
      case DATA: {
	data_header_t * dataheader = (data_header_t *) (((char*) ipv6header) + sizeof(ipv6_header_t));

	/* homa_incoming.c — homa_data_pkt */

	if (homa_rpc.state != homa_rpc_t::RPC_INCOMING) {
	  // Is this a response?
	  if (IS_CLIENT(id)) {
	    // Does this not fit the structure of a response
	    if (homa_rpc.state != homa_rpc_t::RPC_OUTGOING) return;
	    homa_rpc.state = homa_rpc_t::RPC_INCOMING;
	  } else {
	    // if (unlikely(rpc->msgin.total_length >= 0)) goto discard;
	    if (homa_rpc.msgin.total_length >= 0) return;
	  }
	}

	if (homa_rpc.msgin.total_length < 0) {
	  /* homa_message_in_init */

	  //homa_rpc.msgin.total_length = length;
	  //homa_rpc.msgin.bytes_remaining = length;
	  //homa_rpc.incoming = (incoming > length) ? length : incoming;
	  //homa_rpc.priority = 0;
	  //homa_rpc.scheduled = length > incoming;
	}

	/* homa_incoming.c — homa_add_packet */

	// ID to update, packet recieved, max number of packets
	rexmit_touch.write({local_id, 0, 0}); // TODO

	// TODO need to determine whether this data is new and handle partial blocks better
	for (uint32_t seg = 0; seg < ceil(dataheader->seg.segment_length / sizeof(xmit_mblock_t)); ++seg) {
	  // Offset in DMA space, offset in packet space + offset in segment space
	  uint32_t dma_offset = homa_rpc.buffout + dataheader->seg.offset + seg * sizeof(xmit_mblock_t);

	  // Offset of this segment in packet's data block
	  xmit_mblock_t seg_data = *((xmit_mblock_t*) (&(dataheader->seg.data) + (seg * sizeof(xmit_mblock_t))));

	  // TODO this may grab garbage data if the segment runs off the end?

	  dma_egress_reqs.write({dma_offset, seg_data});
	}

	//homa_rpc.total_length -= dataheader->seg.message_length;

	if (homa_rpc.msgin.scheduled) {
	  /* homa_check_grantable */
	  // TODO make this RPC grantable
	}

	// TODO eventually handle cutoffs here

	break;
      }

      case GRANT: {
	// TODO send new grant to the xmit SRPT queue
	break;
      }

      case RESEND: {
	// TODO queue some xmit to the link_egress
	break;
      }

      case BUSY: {
	// TODO don't perform long time out in the timer core
	break;
      }

      case ACK: {
	// TODO update the range of received bytes?
	break;
      }

      case NEED_ACK: {
	// TODO 
	break;
      }

      default:
	break;
    }
  }

  srpt_grant_entry_t entry;
  srpt_grant_queue_insert.write(entry);

  srpt_xmit_entry_t xentry;
  srpt_xmit_queue_update.write(xentry);

  //rpc_id_t touch;
  //rexmit_touch.write(touch);

  // Has a full ethernet frame been buffered? If so, grab it.
  //link_ingress.read(frame);

  // TODO this needs to get the address of the output
  //for (int i = 0; 6; ++i) {
  //  dma_egress_req_t req; //= {i, frame.data[i]};
  //  dma_egress_reqs.write(req);
  //}
}

