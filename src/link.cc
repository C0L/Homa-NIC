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
 * @link_ingress - Raw 64B chunks from the link
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

  //std::cerr << header_in.processed_bytes << std::endl;

  raw_stream_t raw_stream = link_ingress.read();
  //std::cerr << "READ FROM LINK\n";
  // For every type of homa packet we need to read at least two blocks
  if (header_in.processed_bytes == 0) {
    header_in.processed_bytes += 64;
    // TODO perform other checks here (correct ethertype, version, next header)...
    header_in.payload_length = raw_stream.data(511-144,511-159);
    header_in.saddr = raw_stream.data(511-176,511-303);
    header_in.daddr = raw_stream.data(511-304,511-431);
    header_in.sport = raw_stream.data(511-432,511-447);
    header_in.dport = raw_stream.data(511-448,511-463);

    //std::cerr << "READ PORT " << header_in.sport << std::endl;

    //std::cerr << "GOT HEADER BLOCK 0\n";
  } else if (header_in.processed_bytes == 64) {
    header_in.processed_bytes += 64;
    uint8_t type = raw_stream.data(511-24,511-31);
    header_in.type = (homa_packet_type) type;  // Packet type
    uint64_t id = raw_stream.data(511-80,511-143);
    header_in.sender_id = LOCALIZE_ID(id); // Sender RPC ID
    header_in.valid = 1;

    switch(header_in.type) {
      case GRANT: {

	break;
      }

      case DATA: {
	header_in.message_length = (raw_stream.data)(511-144,511-175);   // Length of ENTIRE message
	header_in.incoming = (raw_stream.data)(511-176,511-207); // Expected incoming bytes
	header_in.data_offset = (raw_stream.data)(511-240,511-271);   // Offset in message of segment

	// TODO parse acks

	data_block.buff(511, 400) = (raw_stream.data)(511-400,0);
	data_block.buff(399, 0) = 0;

	//data_block.buff(511, 511-112) = (raw_stream.data)(511-400,0);
	//std::cerr << "PARTIAL BLOCK: " << data_block.buff << std::endl;
	//std::cout << "PARTIAL: " << std::hex << data_block.buff << std::endl;
	data_block.last = raw_stream.last;
	chunk_in_o.write(data_block);

	//std::cerr << "WROTE DATA CHUNK\n";

	data_block.offset += 14;
	
	break;
      }
    }

    //std::cerr << "COMPLETED HEADER BLOCK\n";
    header_in_o.write(header_in);
  } else {
    data_block.buff = raw_stream.data;
    data_block.last = raw_stream.last;
    chunk_in_o.write(data_block);
    //std::cerr << "WROTE PURE DATA CHUNK\n";

    data_block.offset += 64;
  }

  if (raw_stream.last) {
    //std::cerr << "LAST CHUNK RECEIVED\n";
    header_in.processed_bytes = 0;
  }
}
