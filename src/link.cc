#include "link.hh"

#include <iostream>
#include <fstream>

using namespace std;

/**
 * network_order() - Convert the "natural" bit vector to a network order
 * where the LSByte is at bit 7,0 and the MSByte is at bit 511, 504.
 *
 * https://docs.xilinx.com/r/en-US/pg203-cmac-usplus:
 * My understanding is that for the AXI Stream output, 0 is the MSB, and 64
 * is the LSB. The header fields in our core are a bit array where the MSB
 * is the high bit and the LSB is the low bit. We cannot just flip these
 * header fields, as that will change the actual byte value. So, we lay out
 * a 64B block as if byte 64 is the MSB, and then perform a swap operation
 * so that the byte at 64 is now the byte at 0.
 *
 */
void network_order(integral_t & in, integral_t & out) {
#pragma HLS inline
    for (int i = 0; i < 64; ++i) {
#pragma HLS unroll
	out(512 - (i*8) - 1, 512 - 8 - (i*8)) = in(7 + (i*8), (i*8));
    }
}

/**
 * next_pkt_selector() - Chose which of data packets, grant packets, retransmission(TODO)
 * packets, and control packets(TODO) to send next.
 * @data_pkt_i   - Next highest priority data packet to send
 * @grant_pkt_i  - Next highest priority grant packet to send
 * @header_out_o - Output stream to the RPC store to get info about the RPC
 * before the packet can be sent
 */
void next_pkt_selector(hls::stream<srpt_queue_entry_t> & data_pkt_i,
		       hls::stream<srpt_grant_send_t> & grant_pkt_i,
		       hls::stream<header_t> & header_out_o) {

#pragma HLS pipeline II=1

    if (!grant_pkt_i.empty()) {
	srpt_grant_send_t grant_out = grant_pkt_i.read();

	header_t header_out;
	header_out.type         = GRANT;
	header_out.local_id     = grant_out(SRPT_GRANT_SEND_RPC_ID);
	header_out.grant_offset = grant_out(SRPT_GRANT_SEND_MSG_ADDR);
	header_out.packet_bytes = GRANT_PKT_HEADER;

	header_out_o.write(header_out);
    } else if (!data_pkt_i.empty()) {
	srpt_queue_entry_t ready_data_pkt = data_pkt_i.read();

	// TODO why even handle this here?

	header_t header_out;
	header_out.type            = DATA;
	header_out.local_id        = ready_data_pkt(SRPT_QUEUE_ENTRY_RPC_ID);
	header_out.incoming        = ready_data_pkt(SRPT_QUEUE_ENTRY_GRANTED);
	header_out.grant_offset    = ready_data_pkt(SRPT_QUEUE_ENTRY_GRANTED);
	header_out.data_offset     = ready_data_pkt(SRPT_QUEUE_ENTRY_REMAINING);
	header_out.segment_length  = MIN(ready_data_pkt(SRPT_QUEUE_ENTRY_REMAINING), (ap_uint<32>) HOMA_PAYLOAD_SIZE);
	header_out.payload_length  = header_out.segment_length + HOMA_DATA_HEADER;
	header_out.packet_bytes    = DATA_PKT_HEADER + header_out.segment_length;

	header_out_o.write(header_out);
    } 
}

/**
 * pkt_builder() - Take in the packet header data and output
 * structured 64 byte chunks of that packet. Chunks requiring message
 * data will be augmented with that data in the next step. 
 * @header_out_i - Header data that should be sent on the link
 * @chunk_out_o  - 64 byte structured packet chunks output to this stream to
 * be augmented with data from the data buffer
 */
void pkt_builder(hls::stream<header_t> & header_out_i,
		 hls::stream<h2c_chunk_t> & chunk_out_o) {

#pragma HLS pipeline II=1 style=flp

    static header_t header;
    static ap_uint<32> processed_bytes = 0; 
    static bool valid = false;

    // TODO revise the use of valid
    if (valid || (!valid && header_out_i.read_nb(header))) {

	valid = true;

	h2c_chunk_t out_chunk;
	ap_uint<512> natural_chunk;

	out_chunk.msg_addr = header.data_offset;

	switch(header.type) {
	    case DATA: {
		if (processed_bytes == 0) {

		    // Ethernet header
		    natural_chunk(CHUNK_IPV6_MAC_DEST)  = MAC_DST;        // TODO MAC Dest (set by phy?)
		    natural_chunk(CHUNK_IPV6_MAC_SRC)   = MAC_SRC;        // TODO MAC Src (set by phy?)
		    natural_chunk(CHUNK_IPV6_ETHERTYPE) = IPV6_ETHERTYPE; // Ethertype

		    // IPv6 header
		    ap_uint<16> payload_length = (header.packet_bytes - PREFACE_HEADER);

		    natural_chunk(CHUNK_IPV6_VERSION)     = IPV6_VERSION;     // Version
		    natural_chunk(CHUNK_IPV6_TRAFFIC)     = IPV6_TRAFFIC;     // Traffic Class
		    natural_chunk(CHUNK_IPV6_FLOW)        = IPV6_FLOW;        // Flow Label
		    natural_chunk(CHUNK_IPV6_PAYLOAD_LEN) = payload_length;   // Payload Length
		    natural_chunk(CHUNK_IPV6_NEXT_HEADER) = IPPROTO_HOMA;     // Next Header
		    natural_chunk(CHUNK_IPV6_HOP_LIMIT)   = IPV6_HOP_LIMIT;   // Hop Limit
		    natural_chunk(CHUNK_IPV6_SADDR)       = header.saddr; // Sender Address
		    natural_chunk(CHUNK_IPV6_DADDR)       = header.daddr; // Destination Address

		    // Start of common header
		    natural_chunk(CHUNK_HOMA_COMMON_SPORT) = header.sport;  // Sender Port
		    natural_chunk(CHUNK_HOMA_COMMON_DPORT) = header.dport;  // Destination Port

		    // Unused
		    natural_chunk(CHUNK_HOMA_COMMON_UNUSED0) = 0;               // Unused

		    // Packet block configuration — no data bytes needed
		    out_chunk.type  = DATA;
		    out_chunk.width = 0;
		    out_chunk.keep  = 64;
		} else if (processed_bytes == 64) {
		    // Rest of common header
		    natural_chunk(CHUNK_HOMA_COMMON_UNUSED1)   = 0;                    // Unused (2 bytes)
		    natural_chunk(CHUNK_HOMA_COMMON_DOFF)      = DOFF;                 // doff (4 byte chunks in data header)
		    natural_chunk(CHUNK_HOMA_COMMON_TYPE)      = DATA;                 // Type
		    natural_chunk(CHUNK_HOMA_COMMON_UNUSED3)   = 0;                    // Unused (2 bytes)
		    natural_chunk(CHUNK_HOMA_COMMON_CHECKSUM)  = 0;                    // Checksum (unused) (2 bytes)
		    natural_chunk(CHUNK_HOMA_COMMON_UNUSED4)   = 0;                    // Unused  (2 bytes) 
		    natural_chunk(CHUNK_HOMA_COMMON_SENDER_ID) = header.id;            // Sender ID

		    // Data header
		    natural_chunk(CHUNK_HOMA_DATA_MSG_LEN)  = header.message_length; // Message Length (entire message)
		    natural_chunk(CHUNK_HOMA_DATA_INCOMING) = header.incoming;       // Incoming
		    natural_chunk(CHUNK_HOMA_DATA_CUTOFF)   = 0;                     // Cutoff Version (unimplemented) (2 bytes) TODO
		    natural_chunk(CHUNK_HOMA_DATA_REXMIT)   = 0;                     // Retransmit (unimplemented) (1 byte) TODO
		    natural_chunk(CHUNK_HOMA_DATA_PAD)      = 0;                     // Pad (1 byte)

		    // Data Segment
		    natural_chunk(CHUNK_HOMA_DATA_OFFSET)  = header.data_offset;    // Offset
		    natural_chunk(CHUNK_HOMA_DATA_SEG_LEN) = header.segment_length; // Segment Length

		    // Ack header
		    natural_chunk(CHUNK_HOMA_ACK_ID)    = 0;  // Client ID (8 bytes) TODO
		    natural_chunk(CHUNK_HOMA_ACK_SPORT) = 0;  // Client Port (2 bytes) TODO
		    natural_chunk(CHUNK_HOMA_ACK_DPORT) = 0;  // Server Port (2 bytes) TODO

		    // Packet block configuration — 14 data bytes needed
		    out_chunk.type        = DATA;
		    out_chunk.width       = 14;
		    out_chunk.keep        = 64;
		    out_chunk.h2c_buff_id = header.h2c_buff_id;
		    out_chunk.local_id    = header.local_id;
		    header.data_offset    += 14;
		} else {
		    out_chunk.type        = DATA;
		    out_chunk.width       = 64;
		    // TODO revise
		    out_chunk.keep        = ((header.packet_bytes - processed_bytes) < 64) ? ((ap_int<32>) (header.packet_bytes - processed_bytes)) : ((ap_int<32>) 64);
		    out_chunk.h2c_buff_id  = header.h2c_buff_id;
		    out_chunk.local_id = header.local_id;
		    header.data_offset += 64;
		}
		break;
	    }

	    case GRANT: {
		if (processed_bytes == 0) {
		    // TODO this is a little repetetive

		    // Ethernet header
		    natural_chunk(CHUNK_IPV6_MAC_DEST)  = MAC_DST;        // TODO MAC Dest (set by phy?)
		    natural_chunk(CHUNK_IPV6_MAC_SRC)   = MAC_SRC;        // TODO MAC Src (set by phy?)
		    natural_chunk(CHUNK_IPV6_ETHERTYPE) = IPV6_ETHERTYPE; // Ethertype

		    // IPv6 header 
		    ap_uint<16> payload_length = (header.packet_bytes - PREFACE_HEADER);

		    natural_chunk(CHUNK_IPV6_VERSION)        = IPV6_VERSION;     // Version
		    natural_chunk(CHUNK_IPV6_TRAFFIC)        = IPV6_TRAFFIC;     // Traffic Class
		    natural_chunk(CHUNK_IPV6_FLOW)           = IPV6_FLOW;        // Flow Label
		    natural_chunk(CHUNK_IPV6_PAYLOAD_LEN)    = payload_length;   // Payload Length
		    natural_chunk(CHUNK_IPV6_NEXT_HEADER)    = IPPROTO_HOMA;     // Next Header
		    natural_chunk(CHUNK_IPV6_HOP_LIMIT)      = IPV6_HOP_LIMIT;   // Hop Limit
		    natural_chunk(CHUNK_IPV6_SADDR)          = header.saddr;     // Sender Address
		    natural_chunk(CHUNK_IPV6_DADDR)          = header.daddr;     // Destination Address

		    // Start of common header
		    natural_chunk(CHUNK_HOMA_COMMON_SPORT)   = header.sport;  // Sender Port
		    natural_chunk(CHUNK_HOMA_COMMON_DPORT)   = header.dport;  // Destination Port

		    // Unused
		    natural_chunk(CHUNK_HOMA_COMMON_UNUSED0) = 0;             // Unused

		    // Packet block configuration — no data bytes needed
		    out_chunk.type  = GRANT;
		    out_chunk.width = 0;
		} else if (processed_bytes == 64) {
		    // Rest of common header
		    natural_chunk(CHUNK_HOMA_COMMON_UNUSED1)   = 0;                // Unused (2 bytes)
		    natural_chunk(CHUNK_HOMA_COMMON_DOFF)      = DOFF;             // doff (4 byte chunks in data header)
		    natural_chunk(CHUNK_HOMA_COMMON_TYPE)      = GRANT;            // Type
		    natural_chunk(CHUNK_HOMA_COMMON_UNUSED3)   = 0;                // Unused (2 bytes)
		    natural_chunk(CHUNK_HOMA_COMMON_CHECKSUM)  = 0;                // Checksum (unused) (2 bytes)
		    natural_chunk(CHUNK_HOMA_COMMON_UNUSED4)   = 0;                // Unused  (2 bytes) 
		    natural_chunk(CHUNK_HOMA_COMMON_SENDER_ID) = header.id;        // Sender ID

		    // Grant header
		    natural_chunk(CHUNK_HOMA_GRANT_OFFSET)   = header.grant_offset; // Byte offset of grant
		    natural_chunk(CHUNK_HOMA_GRANT_PRIORITY) = 0;                       // Priority TODO

		    // Packet block configuration — no data bytes needed
		    out_chunk.type  = GRANT;
		    out_chunk.width = 0;
		} 
		break;
	    }
	}

	processed_bytes += 64;

	if (processed_bytes >= header.packet_bytes) {
	    processed_bytes = 0;
	    valid = false;
	    out_chunk.last = 1;
	} else {
	    out_chunk.last = 0;
	}

	network_order(natural_chunk, out_chunk.data);
	chunk_out_o.write(out_chunk);
    }
}

/**
 * pkt_chunk_egress() - Take care of final configuration before packet
 * chunks are placed on the link. Set TKEEP and TLAST bits for AXI Stream
 * protocol.
 * TODO this can maybe be absorbed by data buffer core, it just makes less
 * sense for the "databuffer" to be writing to the link
 * @out_chunk_i - Chunks to be output onto the link
 * @link_egress - Outgoign AXI Stream to the link
 */
void pkt_chunk_egress(hls::stream<h2c_chunk_t> & out_chunk_i,
		      hls::stream<raw_stream_t> & link_egress) {
#pragma HLS pipeline
    if (!out_chunk_i.empty()) {

	std::cerr << "pkt chunk egress\n";

	h2c_chunk_t chunk = out_chunk_i.read();
	raw_stream_t raw_stream;

	raw_stream.data = chunk.data;
	raw_stream.last = chunk.last;
	raw_stream.keep = chunk.keep;
	// std::cerr << "OUTPUT TKEEP: " << chunk.keep << std::endl; 
	link_egress.write(raw_stream);
    }
}

/**
 * pkt_chunk_ingress() - Take packet chunks from the link, reconstruct packets,
 * write data to a temporary holding buffer
 * @link_ingress - Raw 64B chunks from the link of incoming packets
 * @header_in_o  - Outgoing stream for incoming headers to peer map
 * @chunk_in_o   - Outgoing stream for storing data until we know
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
		       hls::stream<c2h_chunk_t> & chunk_in_o) {

#pragma HLS pipeline II=1 style=flp

    static header_t header_in;
    static ap_uint<32> processed_bytes = 0;

    if (!link_ingress.empty()) {
	raw_stream_t raw_stream = link_ingress.read();
	std::cerr << "pkt_chunk_ingress\n";
	
	c2h_chunk_t data_block;
	ap_uint<512> natural_chunk;

	network_order(raw_stream.data, natural_chunk);

	// For every type of homa packet we need to read at least two blocks
	if (processed_bytes == 0) {

	    header_in.payload_length = natural_chunk(CHUNK_IPV6_PAYLOAD_LEN);

	    header_in.saddr          = natural_chunk(CHUNK_IPV6_SADDR);
	    header_in.daddr          = natural_chunk(CHUNK_IPV6_DADDR);
	    header_in.sport          = natural_chunk(CHUNK_HOMA_COMMON_SPORT);
	    header_in.dport          = natural_chunk(CHUNK_HOMA_COMMON_DPORT);

	} else if (processed_bytes == 64) {

	    header_in.type = natural_chunk(CHUNK_HOMA_COMMON_TYPE);      // Packet type
	    header_in.id   = natural_chunk(CHUNK_HOMA_COMMON_SENDER_ID); // Sender RPC ID
	    header_in.id   = LOCALIZE_ID(header_in.id);

	    switch(header_in.type) {
		case GRANT: {
		    // Grant header
		    header_in.grant_offset = natural_chunk(CHUNK_HOMA_GRANT_OFFSET);   // Byte offset of grant
		    header_in.priority     = natural_chunk(CHUNK_HOMA_GRANT_PRIORITY); // TODO priority

		    break;
		}

		case DATA: {
		    header_in.message_length = natural_chunk(CHUNK_HOMA_DATA_MSG_LEN);  // Message Length (entire message)
		    header_in.incoming       = natural_chunk(CHUNK_HOMA_DATA_INCOMING); // Expected Incoming Bytes
		    header_in.data_offset    = natural_chunk(CHUNK_HOMA_DATA_OFFSET);   // Offset in message of segment
		    header_in.segment_length = natural_chunk(CHUNK_HOMA_DATA_SEG_LEN);  // Segment Length

		    // TODO parse acks

		    data_block.data(14 * 8, 0) = raw_stream.data(511, 512-14*8);
		    data_block.width  = 14;
		    data_block.last   = raw_stream.last;
		    chunk_in_o.write(data_block);

		    break;
		}
	    }

	    std::cerr << "forwarded header\n";
	    header_in_o.write(header_in);
	} else {
	    data_block.data  = raw_stream.data;
	    data_block.width = raw_stream.keep;
	    data_block.last  = raw_stream.last;

	    chunk_in_o.write(data_block);
	}

	processed_bytes += 64;

	if (raw_stream.last) {
	    processed_bytes = 0;
	}
    }
}
