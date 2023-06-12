#include <string.h>

#include "homa.hh"
#include "link.hh"
#include "dma.hh"

// https://docs.xilinx.com/r/en-US/pg203-cmac-usplus/Typical-Operation

/**
 * egress_selector() - Chose which of data packets, grant packets, retransmission
 * packets, and control packets to send next.
 * @data_pkt_i - Next highest priority data packet input
 * @grant_pkt_i - Next highest priority grant packet input
 * @rexmit_pkt_i - Packets requiring retransmission input
 * @header_out_o - Output stream to the RPC store to get info
 * about the RPC before the packet can be sent
 */
void egress_selector(hls::stream<ready_data_pkt_t> & data_pkt_i,
		     hls::stream<ready_grant_pkt_t> & grant_pkt_i,
		     hls::stream<rexmit_t> & rexmit_pkt_i,
		     hls::stream<header_t> & header_out_o) {

#pragma HLS pipeline II=1

  if (!grant_pkt_i.empty()) {
    DEBUG_MSG("Egress selector grant packet")
    ready_grant_pkt_t ready_grant_pkt = grant_pkt_i.read();

    header_t header_out;
    header_out.type = GRANT;
    header_out.local_id = ready_grant_pkt.rpc_id;
    header_out.grant_offset = ready_grant_pkt.granted;

    header_out.valid = 1;

    header_out_o.write(header_out);

  } else if (!data_pkt_i.empty()) {
    DEBUG_MSG("Egress selector data packet")
    ready_data_pkt_t ready_data_pkt = data_pkt_i.read();

    uint32_t data_bytes = MIN(ready_data_pkt.remaining, HOMA_PAYLOAD_SIZE);
    //uint32_t payload_length = // TODO DATA_PKT_HEADER + data_bytes;

    header_t header_out;
    header_out.type = DATA;
    header_out.local_id = ready_data_pkt.rpc_id;
    header_out.packet_bytes = DATA_PKT_HEADER + data_bytes;
    header_out.payload_length = data_bytes + HOMA_DATA_HEADER;
    header_out.grant_offset = ready_data_pkt.granted;
    header_out.message_length = ready_data_pkt.total;
    header_out.processed_bytes = 0;
    header_out.data_offset = ready_data_pkt.total - ready_data_pkt.remaining;
    header_out.segment_length = data_bytes;
    header_out.valid = 1;

    header_out_o.write(header_out);
  } 
}

/**
 * pkt_chunk_egress() - Take in the packet header data and output
 * structured 64 byte chunks of that packet. Chunks requiring message
 * data will be augmented with that data in the next step. 
 * @rpc_state__chunk_dispatch - Header data input to be sent on the link
 * @chunk_dispatch__dbuff - 64 byte structured packet chunks output to
 * this fifo to be augmented with data from the data buffer
 */
void pkt_chunk_egress(hls::stream<header_t> & header_out_i,
		      hls::stream<out_chunk_t> & chunk_out_o) {

  static header_t doublebuff[2];
#pragma HLS array_partition variable=doublebuff type=complete
#pragma HLS dependence variable=doublebuff intra RAW false
#pragma HLS dependence variable=doublebuff intra WAR false

  static ap_uint<1> w_pkt = 0;
  static ap_uint<1> r_pkt = 0;

  // Need to decouple reading from input stream and reading from current packet
  if (!header_out_i.empty() && doublebuff[w_pkt].valid == 0) {
    doublebuff[w_pkt] = header_out_i.read();
    w_pkt++;
  }

#pragma HLS pipeline II=1

  // We have data to send
  if (doublebuff[r_pkt].valid == 1) {
    header_t & header_out = doublebuff[r_pkt];

    out_chunk_t out_chunk;
    out_chunk.offset = header_out.data_offset;

    switch(header_out.type) {
    case DATA: {
      if (header_out.processed_bytes == 0) {
	DEBUG_MSG("DATA Chunk 0")

	// Ethernet header
	(out_chunk.buff)(511-0,511-47)   = 0xAAAAAAAAAAAA;        // TODO MAC Dest (set by phy?)
	(out_chunk.buff)(511-48,511-95)  = 0xBBBBBBBBBBBB;        // TODO MAC Src (set by phy?)
	(out_chunk.buff)(511-96,511-111) = ETHERTYPE_IPV6;        // Ethertype (fixed)

	// IPv6 header
	(out_chunk.buff)(511-112,511-115) = 0x6;                  // Version (fixed)
	(out_chunk.buff)(511-116,511-123) = 0x00;                 // Traffic Class (left empty)
	(out_chunk.buff)(511-124,511-143) = 0xEEEEE;              // TODO Flow Label ?

	(out_chunk.buff)(511-144,511-159) = header_out.packet_bytes - PREFACE_HEADER; // Payload Length
	(out_chunk.buff)(511-160,511-167) = IPPROTO_HOMA;         // Next Header
	(out_chunk.buff)(511-168,511-175) = 0x0;                  // Hop Limit
	(out_chunk.buff)(511-176,511-303) = header_out.saddr;        // Sender Address
	(out_chunk.buff)(511-304,511-431) = header_out.daddr;        // Destination Address

	// Start of common header
	(out_chunk.buff)(511-432,511-447) = header_out.sport;        // Sender Port
	(out_chunk.buff)(511-448,511-463) = header_out.dport;        // Destination Port
	(out_chunk.buff)(511-464,511-511) = 0xFFFFFFFFFFFF;       // Unused

	// Packet block configuration — no data bytes needed
  	out_chunk.type = DATA;
  	out_chunk.data_bytes = NO_DATA;
	out_chunk.dbuff_id = header_out.dbuff_id;
	header_out.data_offset += NO_DATA;

      } else if (header_out.processed_bytes == 64) {
	DEBUG_MSG("DATA Chunk 1")

	ap_uint<8> doff = 10 << 4;

	// Rest of common header
	(out_chunk.buff)(511-0,511-15)   = 0xFFFF;                // Unused
	(out_chunk.buff)(511-16,511-23)  = doff;                  // doff (4 byte chunks in data header)
	(out_chunk.buff)(511-24,511-31)  = DATA;                  // type
	(out_chunk.buff)(511-32,511-47)  = 0xFFFF;                // Unused
	(out_chunk.buff)(511-48,511-63)  = 0x0;                   // Checksum (unused)
	(out_chunk.buff)(511-64,511-79)  = 0xFFFF;                // Unused
	(out_chunk.buff)(511-80,511-143) = header_out.sender_id;     // Sender ID

	// Data header
	(out_chunk.buff)(511-144,511-175) = header_out.message_length;   // Message Length (entire message)
	(out_chunk.buff)(511-176,511-207) = header_out.incoming; // Incoming TODO 
	(out_chunk.buff)(511-208,511-223) = 0xFFFF;                // Cuttof Version (unimplemented) TODO
	(out_chunk.buff)(511-224,511-231) = 0xFF;                  // Retransmit (unimplemented) TODO
	(out_chunk.buff)(511-232,511-239) = 0xFF;                  // Pad 

	// Data Segment
	(out_chunk.buff)(511-240,511-271) = header_out.data_offset;                    // Offset
	(out_chunk.buff)(511-272,511-303) = header_out.segment_length; // Segment Length

	// Ack header
	(out_chunk.buff)(511-304,511-367) = 0xFFFFFFFFFFFFFFFF; // Client ID
	(out_chunk.buff)(511-368,511-383) = 0xFFFF;             // Client Port
	(out_chunk.buff)(511-384,511-399) = 0xFFFF;             // Server Port

	// Packet block configuration — 14 data bytes needed
  	out_chunk.type = DATA;
  	out_chunk.data_bytes = PARTIAL_DATA;
	out_chunk.dbuff_id = header_out.dbuff_id;
	header_out.data_offset += PARTIAL_DATA;
      } else {
	DEBUG_MSG("DATA Chunk")
	out_chunk.type = DATA;
  	out_chunk.data_bytes = ALL_DATA;
	out_chunk.dbuff_id = header_out.dbuff_id;
	header_out.data_offset += ALL_DATA;
      }
      break;
    }

    case GRANT: {
      if (header_out.processed_bytes == 0) {
	// TODO this is repetitive
	
	// Ethernet header
	(out_chunk.buff)(511-0,511-47)   = 0xAAAAAAAAAAAA;        // TODO MAC Dest (set by phy?)
	(out_chunk.buff)(511-48,511-95)  = 0xBBBBBBBBBBBB;        // TODO MAC Src (set by phy?)
	(out_chunk.buff)(511-96,511-111) = ETHERTYPE_IPV6;        // Ethertype (fixed)

	// IPv6 header
	(out_chunk.buff)(511-112,511-115) = 0x6;                  // Version (fixed)
	(out_chunk.buff)(511-116,511-123) = 0x00;                 // Traffic Class (left empty)
	(out_chunk.buff)(511-124,511-143) = 0xEEEEE;              // TODO Flow Label ?

	(out_chunk.buff)(511-144,511-159) = header_out.payload_length; // Payload Length
	(out_chunk.buff)(511-160,511-167) = IPPROTO_HOMA;            // Next Header
	(out_chunk.buff)(511-168,511-175) = 0x0;                     // Hop Limit
	(out_chunk.buff)(511-176,511-303) = header_out.saddr;        // Sender Address
	(out_chunk.buff)(511-304,511-431) = header_out.daddr;        // Destination Address

	// Start of common header
	(out_chunk.buff)(511-432,511-447) = header_out.sport;        // Sender Port
	(out_chunk.buff)(511-448,511-463) = header_out.dport;        // Destination Port
	(out_chunk.buff)(511-464,511-511) = 0xFFFFFFFFFFFF;          // Unused

	// Packet block configuration — no data bytes needed
  	out_chunk.type = GRANT;
  	out_chunk.data_bytes = NO_DATA;

      } else if (header_out.processed_bytes == 64) {

	ap_uint<8> doff = 10 << 4;

	// Rest of common header
	(out_chunk.buff)(511-0,511-15)   = 0xFFFF;                // Unused
	(out_chunk.buff)(511-16,511-23)  = doff;                  // doff (4 byte chunks in data header)
	(out_chunk.buff)(511-24,511-31)  = DATA;                  // type
	(out_chunk.buff)(511-32,511-47)  = 0xFFFF;                // Unused
	(out_chunk.buff)(511-48,511-63)  = 0x0;                   // Checksum (unused)
	(out_chunk.buff)(511-64,511-79)  = 0xFFFF;                // Unused
	(out_chunk.buff)(511-80,511-143) = header_out.sender_id;     // Sender ID

	// Grant Header
	(out_chunk.buff)(511-144,511-175) = header_out.message_length; // Message Length (entire message)
	(out_chunk.buff)(511-176,511-183) = 0xFF;                      // Priority TODO 

	// Packet block configuration — no data bytes needed
  	out_chunk.type = GRANT;
  	out_chunk.data_bytes = NO_DATA;
      } 
    }
    }

    header_out.processed_bytes += 64;

    if (header_out.processed_bytes >= header_out.packet_bytes) {
      header_out.valid = 0;
      out_chunk.last = 1;
      r_pkt++;
    }

    chunk_out_o.write(out_chunk);
  }
}

/**
 * pkt_chunk_ingress() - Take packet chunks from the link, reconstruct packets,
 * write data to a temporary holding buffer
 * @link_egress - Raw 64B chunks from the link
 * @chunk_ingress__peer_map - Outgoing stream for incoming headers to peer map
 * @chunk_ingress__data_stageing  - Outgoing stream for storing data until we know
 * where to place it in DMA space
 *
 * We can't just write directly to DMA space as we don't know where until
 * we have performed the reverse mapping to get a local RPC ID.
 *
 * Could alternatively send all packets through the same path but this approach
 * seems simpler
 */
void pkt_chunk_ingress(hls::stream<raw_stream_t> & link_ingress,
		       hls::stream<header_t> & header_in_o,
		       hls::stream<in_chunk_t> & chunk_in_o) {
#pragma HLS pipeline II=1
  // TODO this gradual parsing approach feels awk. Could build the packet in memory and then do it all at once?

  static header_t header_in;
  static in_chunk_t data_block;

  raw_stream_t raw_stream = link_ingress.read();

  // For every type of homa packet we need to read at least two blocks
  if (header_in.processed_bytes == 0) {
    header_in.processed_bytes += 64;
    // TODO perform other checks here (correct ethertype, version, next header)...
    header_in.payload_length = raw_stream.data(511-144,511-159);
    header_in.saddr = raw_stream.data(511-176,511-303);
    header_in.daddr = raw_stream.data(511-304,511-431);
    header_in.sport = raw_stream.data(511-432,511-447);
    header_in.dport = raw_stream.data(511-448,511-463);
  } else if (header_in.processed_bytes == 64) {
    header_in.processed_bytes += 64;
    uint8_t type = raw_stream.data(511-24,511-31);
    header_in.type = (homa_packet_type) type;  // Packet type
    uint64_t id = raw_stream.data(511-80,511-143);
    header_in.sender_id = LOCALIZE_ID(id); // Sender RPC ID

    switch(header_in.type) {
      case GRANT: {

	break;
      }

      case DATA: {
	header_in.message_length = (raw_stream.data)(511-144,511-175);   // Length of ENTIRE message
	header_in.incoming = (raw_stream.data)(511-176,511-207); // Expected incoming bytes
	header_in.data_offset = (raw_stream.data)(511-240,511-271);   // Offset in message of segment

	// TODO parse acks
	data_block.buff(511, 511-112) = (raw_stream.data)(511-400,0);
	data_block.last = raw_stream.last;
	chunk_in_o.write(data_block);

	data_block.offset += 14;
	
	break;
      }
    }

    header_in_o.write(header_in);
  } else {
    data_block.buff = raw_stream.data;
    data_block.last = raw_stream.last;
    chunk_in_o.write(data_block);

    data_block.offset += 64;
  }

  if (raw_stream.last) {
    header_in.processed_bytes = 0;
  }
}



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
////}
