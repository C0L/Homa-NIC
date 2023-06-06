#include <string.h>

#include "homa.hh"
#include "link.hh"
#include "dma.hh"

// Initiate an outgoing packet
void egress_selector(hls::stream<srpt_data_t> & data_pkt_i,
		     hls::stream<srpt_grant_t> & grant_pkt_i,
		     hls::stream<rexmit_t> & rexmit_pkt_i,
		     hls::stream<out_pkt_t> & out_pkt_o) {

#pragma HLS pipeline II=1

  // TODO prioritize grants

  if (!data_pkt_i.empty()) {
    DEBUG_MSG("Egress selector data packet")
    srpt_data_t next_data_pkt = data_pkt_i.read();

    uint32_t data_bytes = MIN(next_data_pkt.remaining, HOMA_PAYLOAD_SIZE);
    uint32_t total_bytes = DATA_PKT_HEADER + data_bytes;

    out_pkt_t out_pkt;
    out_pkt.type = DATA;
    out_pkt.rpc_id = next_data_pkt.rpc_id;
    out_pkt.total_bytes = total_bytes;
    out_pkt.sent_bytes = 0;
    out_pkt.data_offset = next_data_pkt.total - next_data_pkt.remaining;
    out_pkt.valid = 1;

    out_pkt_o.write(out_pkt);
  }

  //else if (!srpt_grant_queue_next_in.empty()) {
  //  srpt_grant_entry_t next_grant_pkt = srpt_grant_queue_next_in.read();

  //  // TODO 
  //  pending_pkt_t pending_pkt;
  //  pending_pkt.type = GRANT;

  //  pending_pkt_out.write(pending_pkt);
  //} else if (!rexmit_rpcs_in.empty()) {
  //  rexmit_t rexmit = rexmit_rpcs_in.read();

  //  // TODO 
  //  pending_pkt_t pending_pkt;
  //  pending_pkt.type = RESEND;

  //  pending_pkt_out.write(pending_pkt);
  //}
}


void pkt_chunk_dispatch(hls::stream<out_pkt_t> & rpc_state__chunk_dispatch,
			hls::stream<out_block_t> & chunk_dispatch__dbuff) {
  static out_pkt_t out_pkt;
  
#pragma HLS pipeline II=1

  if (out_pkt.valid == 1 && out_pkt.sent_bytes < out_pkt.total_bytes) {
    //std::cerr << "DEBUG: Send chunk" << std::endl;
    out_block_t out_block;

    if ((out_pkt.sent_bytes + 64) >= out_pkt.total_bytes) {
      out_block.last = 1;
      out_pkt.valid = 0;
    }

    switch(out_pkt.type) {
      case DATA: {
  	if (out_pkt.sent_bytes == 0) {
	  DEBUG_MSG("DATA Chunk 0")
	  // Packet block configuration — no data bytes needed
  	  out_block.type = DATA;
  	  out_block.data_bytes = NO_DATA;
	  out_block.dbuff_id = out_pkt.dbuff_id;
	  out_pkt.data_offset += NO_DATA;

	  //ap_uint<512> * block = (ap_uint<512> *) out_block.buff;

	  (out_block.buff)(48,0) = 0xAAAAAAAAAAAA;
	  (out_block.buff)(96,48) = 0xBBBBBBBBBBBB;
  	  //data_block_0_t * block = (data_block_0_t *) out_block.buff;

	  // Ethernet header
	  //block->dest_mac = 0xAAAAAAAAAAAA;
	  //block->src_mac  = 0xBBBBBBBBBBBB;
	  //block->ethertype = ETHERTYPE_IPV6;

	  // IPv6 header
	  //block->version = 0xC;
	  //block->traffic_class = 0xDD;
	  //block->flow_label = 0xEEEEE;
	  //block->next_header = IPPROTO_HOMA;
	  //block->hop_limit = 0xFF;
	  //block->src_address = out_pkt.saddr;
	  //block->dest_address = out_pkt.daddr;

	  //// Start of common header
	  //block->sport = out_pkt.sport;
	  //block->dport = out_pkt.dport;
	  //block->unused = 0;
  	} else if (out_pkt.sent_bytes == 64) {
	  DEBUG_MSG("DATA Chunk 1")
	  // Packet block configuration — 14 data bytes needed
  	  out_block.type = DATA;
  	  out_block.data_bytes = PARTIAL_DATA;
	  out_block.dbuff_id = out_pkt.dbuff_id;
	  out_pkt.data_offset += PARTIAL_DATA;

  	  data_block_1_t * block = (data_block_1_t *) out_block.buff;

	  // Remaining bytes of common header
	  block->unused0 = 0;
	  block->doff = 0xFFFFFFFF;
	  block->type = DATA;
	  block->unused1 = 0;
	  block->checksum = 0;
	  block->unused2 = 0;
	  block->sender_id = out_pkt.rpc_id;

	  // Data header
	  block->message_length = 0xFFFFFFFF;
	  block->incoming = 0xFFFFFFFF;
	  block->cutoff_version = 0xFFFFFFFF;
	  block->retransmit = 0xFFFFFFFF;
	  block->pad = 0;
	  block->offset = 0xFFFFFFFF;
	  block->segment_length = 0xFFFFFFFF;

	  // Ack header
	  block->client_id = 0;
	  block->client_port = 0;
	  block->server_port = 0;

  	} else {
	  DEBUG_MSG("DATA Chunk")
	  out_block.type = DATA;
  	  out_block.data_bytes = ALL_DATA;
	  out_block.dbuff_id = out_pkt.dbuff_id;
	  out_pkt.data_offset += ALL_DATA;
	}
      } 
    }

    out_block.offset = out_pkt.data_offset;

    out_pkt.sent_bytes += 64;
    //out_pkt.data_offset += 64;
    chunk_dispatch__dbuff.write(out_block);
  }

  if (out_pkt.valid == 0 && !rpc_state__chunk_dispatch.empty()) {
    rpc_state__chunk_dispatch.read(out_pkt);
    DEBUG_MSG("Read new packet " << out_pkt.data_offset)
  } 
}

// Convert packet streams to the outgoing link
//void link_egress(stream_t<out_block_t, raw_stream_t> & out_pkt_s) {
//
//		 //hls::stream<out_block_t> & out_block_in,
//		 //hls::stream<raw_stream_t> & link_egress_out) {
//
//#pragma HLS pipeline II=1
//  out_block_t data_pkt = out_pkt_s.in.read();
//  raw_stream_t raw_chunk;
//
//  for (int i = 0; i < 64; ++i) {
//#pragma HLS unroll
//    raw_chunk.data[i] = data_pkt.buff[i];
//  }
//
//  // TODO needs to check if last packet in sequence and set tlast if so
//  if (data_pkt.done == 1) {
//    raw_chunk.last = 1;
//  }
//
//  out_pkt_s.out.write(raw_chunk);
//}




/**
 * proc_link_ingress() - 
 * 
 */
void link_ingress(hls::stream<raw_frame_t> & link_ingress_in,
		  hls::stream<srpt_grant_t> & srpt_grant_queue_insert,
		  hls::stream<srpt_data_t> & srpt_xmit_queue_update,
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
		  hls::stream<touch_t> & rexmit_touch,
		  hls::stream<dma_egress_req_t> & dma_egress_reqs) {

  //#pragma HLS pipeline II=1
  //
  //  /*
  //   * Are there incoming ethernet frames to process
  //   * Raw frame max size for non jumbo frames:
  //   *   6 + 6 + 4 + 2 + 1500 + 4 = 1522 bytes 
  //   */
  //  if (!link_ingress_in.empty()) {
  //    raw_frame_t raw_frame;
  //    link_ingress_in.read(raw_frame);
  //
  //    ethernet_header_t * ethheader = (ethernet_header_t*) &(raw_frame.data);
  //
  //    if (ethheader->ethertype != ETHERTYPE_IPV6) return;
  //
  //    ipv6_header_t * ipv6header = (ipv6_header_t*) (((char*) ethheader) + sizeof(ethernet_header_t));
  //
  //    // Version is a constant that should always be 6? 
  //    if (ipv6header->version != 0x6) return;
  //
  //    // TODO traffic class? Nothing to do here?
  //
  //    // TODO flow label? Nothing to do here?
  //
  //    int payload_length = ipv6header->payload_length;
  //
  //    if (ipv6header->next_header != IPPROTO_HOMA) return;
  //
  //    // TODO Hop limit? Nothing to do here?
  //
  //    // TODO Dest address? Nothing to do here?
  //
  //    common_header_t * commonheader = (common_header_t *) (((char*) ipv6header) + sizeof(ipv6_header_t));
  //
  //    /* homa_incoming.c — homa_pkt_dispatch() */
  //
  //    uint64_t id = LOCALIZE_ID(commonheader->sender_id);
  //
  //    // TODO there could be an ACK in the packet? check homa_incoming.c
  //
  //    // A meaningful value in the rpc_buffer
  //    rpc_id_t local_id;
  //    homa_rpc_t homa_rpc;
  //   
  //    if (!IS_CLIENT(id)) {
  //      /* homa_find_server_rpc */
  //
  //      // Search tuple for local rpc data from server rpc
  //      rpc_hashpack_t rpc_hashpack = {
  //	ipv6header->src_address,
  //	id,
  //	commonheader->sport,
  //	0
  //      };
  //
  //      // Search for associated RPC ID
  //      rpc_table_request.write(rpc_hashpack);
  //      rpc_table_response.read(local_id);
  //
  //      if (commonheader->doff == DATA) {
  //	/* homa_rpc_new_server */
  //
  //	// Did the search fail? If so create a nw rpc
  //	if (local_id == 0) { 
  //	  homa_rpc.state = homa_rpc_t::RPC_INCOMING;
  //
  //	  // TODO set flags/grants in progress?
  //
  //	  /* homa_peer_find */
  //
  //	  /* Equivalent to homa_peer_find */
  //	  peer_hashpack_t peer_hashpack = {ipv6header->src_address};
  //	  peer_table_request.write(peer_hashpack); 
  //	  peer_table_response.read(homa_rpc.peer_id);
  //	  
  //	  // If the peer table request returned 0, then no peer is found 
  //	  if (homa_rpc.peer_id == 0) {
  //	    // Get an available peer ID (an index into the peer buffer)
  //	    peer_stack_next.read(homa_rpc.peer_id);
  //	    
  //	    // Create a new peer to add into the peer buffer
  //	    homa_peer_t peer;
  //	    // TODO: there will eventually be other entries in the homa_peer_t
  //	    peer.peer_id = homa_rpc.peer_id;
  //	    peer.addr = ipv6header->src_address;
  //
  //	    // Store the peer data
  //	    peer_buffer_insert.write(peer);
  //	    
  //	    // Store the mapping from IP addres to peer
  //	    peer_table_insert.write(peer);
  //	  } 
  //
  //	  homa_rpc.daddr = ipv6header->src_address;
  //	  homa_rpc.dport = commonheader->sport;
  //	  homa_rpc.completion_cookie = 0; // TODO irrelevent for server side
  //	  homa_rpc.msgin.total_length = -1;
  //
  //	  // TODO homa_rpc.msgout?
  //
  //	  homa_rpc.msgout.length = -1;
  //	  homa_rpc.silent_ticks = 0;
  //	  // TODO homa_rpc.resend_timer_ticks = homa->timer_ticks;
  //	  homa_rpc.done_timer_ticks = 0;
  //
  //	  // Get a new local ID
  //	  rpc_stack_next.read(local_id);
  //	  homa_rpc.rpc_id = local_id;
  //
  //	  rpc_buffer_insert.write(homa_rpc);
  //
  //	  // Create a mapping from src address + id + sport -> local id
  //	  rpc_table_insert.write(homa_rpc);
  //	} else {
  //	  rpc_buffer_request.write(local_id);
  //	  rpc_buffer_response.read(homa_rpc);
  //	}
  //      }
  //    } else {
  //      // Client RPCs are meaningful in the rpc_buffer
  //      local_id = id;
  //    }
  //
  //    switch(commonheader->doff) {
  //      case DATA: {
  //	data_header_t * dataheader = (data_header_t *) (((char*) ipv6header) + sizeof(ipv6_header_t));
  //
  //	/* homa_incoming.c — homa_data_pkt */
  //
  //	if (homa_rpc.state != homa_rpc_t::RPC_INCOMING) {
  //	  // Is this a response?
  //	  if (IS_CLIENT(id)) {
  //	    // Does this not fit the structure of a response
  //	    if (homa_rpc.state != homa_rpc_t::RPC_OUTGOING) return;
  //	    homa_rpc.state = homa_rpc_t::RPC_INCOMING;
  //	  } else {
  //	    // if (unlikely(rpc->msgin.total_length >= 0)) goto discard;
  //	    if (homa_rpc.msgin.total_length >= 0) return;
  //	  }
  //	}
  //
  //	if (homa_rpc.msgin.total_length < 0) {
  //	  /* homa_message_in_init */
  //
  //	  //homa_rpc.msgin.total_length = length;
  //	  //homa_rpc.msgin.bytes_remaining = length;
  //	  //homa_rpc.incoming = (incoming > length) ? length : incoming;
  //	  //homa_rpc.priority = 0;
  //	  //homa_rpc.scheduled = length > incoming;
  //	}
  //
  //	/* homa_incoming.c — homa_add_packet */
  //
  //	// ID to update, packet recieved, max number of packets
  //	rexmit_touch.write({local_id, true, 0, 0}); // TODO
  //
  //	// TODO need to determine whether this data is new and handle partial blocks better
  //	for (uint32_t seg = 0; seg < ceil(dataheader->seg.segment_length / sizeof(dbuff_block_t)); ++seg) {
  //	  // Offset in DMA space, offset in packet space + offset in segment space
  //	  uint32_t dma_offset = homa_rpc.buffout + dataheader->seg.offset + seg * sizeof(xmit_mblock_t);
  //
  //	  // Offset of this segment in packet's data block
  //	  xmit_mblock_t seg_data = *((xmit_mblock_t*) (&(dataheader->seg.data) + (seg * sizeof(xmit_mblock_t))));
  //
  //	  // TODO this may grab garbage data if the segment runs off the end?
  //
  //	  dma_egress_reqs.write({dma_offset, seg_data});
  //	}
  //
  //	//homa_rpc.total_length -= dataheader->seg.message_length;
  //
  //	if (homa_rpc.msgin.scheduled) {
  //	  /* homa_check_grantable */
  //	  // TODO make this RPC grantable
  //	}
  //
  //	// TODO eventually handle cutoffs here
  //
  //	break;
  //      }
  //
  //      case GRANT: {
  //	// TODO send new grant to the xmit SRPT queue
  //	break;
  //      }
  //
  //      case RESEND: {
  //	// TODO queue some xmit to the link_egress
  //	break;
  //      }
  //
  //      case BUSY: {
  //	// TODO don't perform long time out in the timer core
  //	break;
  //      }
  //
  //      case ACK: {
  //	// TODO update the range of received bytes?
  //	break;
  //      }
  //
  //      case NEED_ACK: {
  //	// TODO 
  //	break;
  //      }
  //
  //      default:
  //	break;
  //    }
  //  }
  //
  //  srpt_grant_entry_t entry;
  //  srpt_grant_queue_insert.write(entry);
  //
  //  srpt_xmit_entry_t xentry;
  //  srpt_xmit_queue_update.write(xentry);
  //
  //  //rpc_id_t touch;
  //  //rexmit_touch.write(touch);
  //
  //  // Has a full ethernet frame been buffered? If so, grab it.
  //  //link_ingress.read(frame);
  //
  //  // TODO this needs to get the address of the output
  //  //for (int i = 0; 6; ++i) {
  //  //  dma_egress_req_t req; //= {i, frame.data[i]};
  //  //  dma_egress_reqs.write(req);
  //  //}
}
