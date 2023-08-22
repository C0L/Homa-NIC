#include "link.hh"

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
 * egress_selector() - Chose which of data packets, grant packets, retransmission(TODO)
 * packets, and control packets(TODO) to send next.
 * @data_pkt_i   - Next highest priority data packet to send
 * @grant_pkt_i  - Next highest priority grant packet to send
 * @header_out_o - Output stream to the RPC store to get info about the RPC
 * before the packet can be sent
 */
void egress_selector(hls::stream<srpt_pktq_t> & data_pkt_i,
		     hls::stream<srpt_grant_out_t> & grant_pkt_i,
		     hls::stream<header_t> & header_out_o) {

#pragma HLS pipeline II=1

    if (!grant_pkt_i.empty()) {
	srpt_grant_out_t grant_out = grant_pkt_i.read();

	header_t header_out;
	header_out.type         = GRANT;
	header_out.local_id     = grant_out(SRPT_GRANT_OUT_RPC_ID);
	header_out.grant_offset = grant_out(SRPT_GRANT_OUT_GRANT);
	header_out.packet_bytes = GRANT_PKT_HEADER;

	header_out_o.write(header_out);
    } else if (!data_pkt_i.empty()) {
	srpt_pktq_t ready_data_pkt = data_pkt_i.read();

	// TODO why even handle this here?

	header_t header_out;
	header_out.type            = DATA;
	header_out.local_id        = ready_data_pkt(PKTQ_RPC_ID);
	header_out.incoming        = ready_data_pkt(PKTQ_GRANTED);
	header_out.grant_offset    = ready_data_pkt(PKTQ_GRANTED);
	header_out.data_offset     = ready_data_pkt(PKTQ_REMAINING);
	header_out.segment_length  = MIN(ready_data_pkt(PKTQ_REMAINING), (ap_uint<32>) HOMA_PAYLOAD_SIZE);
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
		 hls::stream<out_chunk_t> & chunk_out_o) {

#pragma HLS pipeline II=1 style=flp

    static header_t header;
    static ap_uint<32> processed_bytes = 0; 
    static bool valid = false;

    if (valid || (!valid && header_out_i.read_nb(header))) {

	valid = true;

	out_chunk_t out_chunk;
	ap_uint<512> natural_chunk;

	out_chunk.offset = header.data_offset;

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
		    out_chunk.type = DATA;
		    out_chunk.data_bytes = NO_DATA;
		    // out_chunk.dbuff_id = header.obuff_id;
		    // out_chunk.local_id = header.local_id;
		    header.data_offset += NO_DATA;

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
		    natural_chunk(CHUNK_HOMA_DATA_CUTOFF)   = 0;                         // Cutoff Version (unimplemented) (2 bytes) TODO
		    natural_chunk(CHUNK_HOMA_DATA_REXMIT)   = 0;                         // Retransmit (unimplemented) (1 byte) TODO
		    natural_chunk(CHUNK_HOMA_DATA_PAD)      = 0;                         // Pad (1 byte)

		    // Data Segment
		    natural_chunk(CHUNK_HOMA_DATA_OFFSET)  = header.data_offset;    // Offset
		    natural_chunk(CHUNK_HOMA_DATA_SEG_LEN) = header.segment_length; // Segment Length
		    std::cerr << "SENT CHUNK OFFSET " << header.data_offset << std::endl;

		    // Ack header
		    natural_chunk(CHUNK_HOMA_ACK_ID)    = 0;  // Client ID (8 bytes) TODO
		    natural_chunk(CHUNK_HOMA_ACK_SPORT) = 0;  // Client Port (2 bytes) TODO
		    natural_chunk(CHUNK_HOMA_ACK_DPORT) = 0;  // Server Port (2 bytes) TODO

		    // Packet block configuration — 14 data bytes needed
		    out_chunk.type = DATA;
		    out_chunk.data_bytes = PARTIAL_DATA;
		    out_chunk.length   = header.message_length;
		    out_chunk.dbuff_id = header.obuff_id;
		    out_chunk.local_id = header.local_id;
		    header.data_offset += PARTIAL_DATA;
		} else {
		    out_chunk.type = DATA;
		    out_chunk.data_bytes = ALL_DATA;
		    out_chunk.length   = header.message_length;
		    out_chunk.dbuff_id = header.obuff_id;
		    out_chunk.dbuff_id = header.obuff_id;
		    out_chunk.local_id = header.local_id;
		    header.data_offset += ALL_DATA;
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
		    natural_chunk(CHUNK_IPV6_HOP_LIMIT)      = HOP_LIMIT;        // Hop Limit
		    natural_chunk(CHUNK_IPV6_SADDR)          = header.saddr; // Sender Address
		    natural_chunk(CHUNK_IPV6_DADDR)          = header.daddr; // Destination Address

		    // Start of common header
		    natural_chunk(CHUNK_HOMA_COMMON_SPORT)   = header.sport;  // Sender Port
		    natural_chunk(CHUNK_HOMA_COMMON_DPORT)   = header.dport;  // Destination Port

		    // Unused
		    natural_chunk(CHUNK_HOMA_COMMON_UNUSED0) = 0;             // Unused

		    // Packet block configuration — no data bytes needed
		    out_chunk.type = GRANT;
		    out_chunk.data_bytes = NO_DATA;
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
		    out_chunk.type = GRANT;
		    out_chunk.data_bytes = NO_DATA;
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

	network_order(natural_chunk, out_chunk.buff);
	std::cerr << "WROTE CHUNK OUT " << processed_bytes << std::endl;
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

#ifdef STEPPED
void pkt_chunk_egress(bool egress_en,
		      hls::stream<out_chunk_t> & out_chunk_i,
		      hls::stream<raw_stream_t> & link_egress) {
#else
void pkt_chunk_egress(hls::stream<out_chunk_t> & out_chunk_i,
		      hls::stream<raw_stream_t> & link_egress) {
#endif

    #ifdef STEPPED
    if (egress_en) {
    #endif
    out_chunk_t chunk = out_chunk_i.read();
    raw_stream_t raw_stream;
    // network_order(chunk.buff, raw_stream.data);
    raw_stream.data = chunk.buff;
    raw_stream.last = chunk.last;
    link_egress.write(raw_stream);
    #ifdef STEPPED
    }
    #endif
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
#ifdef STEPPED
void pkt_chunk_ingress(bool ingress_en,
		       hls::stream<raw_stream_t> & link_ingress,
		       hls::stream<header_t> & header_in_o,
		       hls::stream<in_chunk_t> & chunk_in_o) {
#else
void pkt_chunk_ingress(hls::stream<raw_stream_t> & link_ingress,
		       hls::stream<header_t> & header_in_o,
		       hls::stream<in_chunk_t> & chunk_in_o) {
#endif

#pragma HLS pipeline II=1 style=flp

    static header_t header_in;
    static ap_uint<32> processed_bytes = 0;

#ifdef STEPPED
    if (ingress_en) {
#endif

	//if (!chunk_in_o.full() && !header_in_o.full()) {
	    raw_stream_t raw_stream = link_ingress.read();

	    std::cerr << "READ CHUNK IN " << processed_bytes << std::endl;

	    in_chunk_t data_block;
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

			std::cerr << "READ DATA OFFSET " << header_in.data_offset << std::endl;

			// TODO parse acks

			data_block.buff(PARTIAL_DATA*8, 0) = raw_stream.data(511, 512-PARTIAL_DATA*8);
			data_block.offset = 0;
			data_block.last   = raw_stream.last;
			chunk_in_o.write(data_block);

			break;
		    }
		}

		std::cerr << "header in write\n";
		header_in_o.write(header_in);
		std::cerr << "complete\n";
	    } else {
		// TODO need to test TKEEP and change raw_stream def

		data_block.offset = processed_bytes - DATA_PKT_HEADER;
		data_block.buff   = raw_stream.data;
		data_block.last   = raw_stream.last;

		chunk_in_o.write(data_block);
	    }

	    processed_bytes += 64;

	    if (raw_stream.last) {
		std::cerr << "LB RECV\n";
		processed_bytes = 0;
	    }
	    // }

#ifdef STEPPED
    }
#endif

}
