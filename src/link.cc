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
    ready_grant_pkt_t ready_grant_pkt = grant_pkt_i.read();

    header_t header_out;
    header_out.type = GRANT;
    header_out.local_id = ready_grant_pkt.rpc_id;
    header_out.grant_offset = ready_grant_pkt.grantable;

    header_out.valid = 1;

    header_out_o.write(header_out);

  } else if (!data_pkt_i.empty()) {
    ready_data_pkt_t ready_data_pkt = data_pkt_i.read();

    uint32_t data_bytes = MIN(ready_data_pkt.remaining, HOMA_PAYLOAD_SIZE);

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
    header_out.incoming = ready_data_pkt.granted;
    header_out.valid = 1;

    header_out_o.write(header_out);
  } 
}

template<int W>
void htno_set(ap_uint<W*8> & out, const ap_uint<W*8> & in) {
#pragma HLS inline 
  for (int i = 0; i < W; ++i) {
#pragma HLS unroll
    out((W * 8) - 1 - (i * 8), (W*8) - 8 - (i * 8)) = in(7 + (i * 8), 0 + (i * 8));
  }
}

/**
 * pkt_chunk_egress() - Take in the packet header data and output
 * structured 64 byte chunks of that packet. Chunks requiring message
 * data will be augmented with that data in the next step. 
 * @header_out_i - Header data that should be sent on the link
 * @chunk_out_o - 64 byte structured packet chunks output to
 * this stream to be augmented with data from the data buffer
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

	  // Ethernet header
	  htno_set<6>(*((ap_uint<48>*) (out_chunk.buff.data)), MAC_DST); // TODO MAC Dest (set by phy?)
	  htno_set<6>(*((ap_uint<48>*) (out_chunk.buff.data + 6)), MAC_SRC); // TODO MAC Src (set by phy?)
	  htno_set<2>(*((ap_uint<16>*) (out_chunk.buff.data + 12)), ETHERTYPE_IPV6); // Ethertype

	  // IPv6 header
	  htno_set<4>(*((ap_uint<32>*) (out_chunk.buff.data + 14)), VTF); // Version/Traffic Class/Flow Label
	  htno_set<2>(*((ap_uint<16>*) (out_chunk.buff.data + 18)), header_out.packet_bytes - PREFACE_HEADER); // Payload Length
	  htno_set<1>(*((ap_uint<8>*) (out_chunk.buff.data + 20)), IPPROTO_HOMA); // Next Header
	  htno_set<1>(*((ap_uint<8>*) (out_chunk.buff.data + 21)), HOP_LIMIT); // Hop Limit
	  htno_set<16>(*((ap_uint<128>*) (out_chunk.buff.data + 22)), header_out.saddr); // Sender Address
	  htno_set<16>(*((ap_uint<128>*) (out_chunk.buff.data + 38)), header_out.daddr); // Destination Address

	  // Start of common header
	  htno_set<2>(*((ap_uint<16>*) (out_chunk.buff.data + 54)), header_out.sport); // Sender Port
	  htno_set<2>(*((ap_uint<16>*) (out_chunk.buff.data + 56)), header_out.dport); // Destination Port
	  // Unused

	  // Packet block configuration — no data bytes needed
	  out_chunk.type = DATA;
	  out_chunk.data_bytes = NO_DATA;
	  out_chunk.dbuff_id = header_out.dbuff_id;
	  header_out.data_offset += NO_DATA;

        } else if (header_out.processed_bytes == 64) {
	  // Rest of common header
	  *((ap_uint<16>*) (out_chunk.buff.data)) = 0; // Unused (2 bytes)
	  htno_set<1>(*((ap_uint<8>*) (out_chunk.buff.data+2)), DOFF);      // doff (4 byte chunks in data header)
	  htno_set<1>(*((ap_uint<8>*) (out_chunk.buff.data+3)), DATA_TYPE); // Type
	  *((ap_uint<16>*) (out_chunk.buff.data+4)) = 0; // Unused (2 bytes)
	  *((ap_uint<16>*) (out_chunk.buff.data+6)) = 0; // Checksum (unused) (2 bytes)
	  *((ap_uint<16>*) (out_chunk.buff.data+8)) = 0; // Unused  (2 bytes)
	  htno_set<8>(*((ap_uint<64>*) (out_chunk.buff.data+10)), header_out.sender_id); // Sender ID

	  // Data header
	  htno_set<4>(*((ap_uint<32>*) (out_chunk.buff.data+18)), header_out.message_length); // Message Length (entire message)
	  htno_set<4>(*((ap_uint<32>*) (out_chunk.buff.data+22)), header_out.incoming); // Incoming
	  *((ap_uint<16>*) (out_chunk.buff.data+26)) = 0; // Cutoff Version (unimplemented) (2 bytes) TODO
	  *((ap_uint<8>*) (out_chunk.buff.data+27)) = 0; // Retransmit (unimplemented) (1 byte) TODO
	  *((ap_uint<8>*) (out_chunk.buff.data+28)) = 0; // Pad (1 byte)

	  // Data Segment
	  htno_set<4>(*((ap_uint<32>*) (out_chunk.buff.data+29)), header_out.data_offset);    // Offset
	  htno_set<4>(*((ap_uint<32>*) (out_chunk.buff.data+33)), header_out.segment_length); // Segment Length

	  // Ack header
	  *((ap_uint<64>*) (out_chunk.buff.data+37)) = 0; // Client ID (8 bytes) TODO
	  *((ap_uint<16>*) (out_chunk.buff.data+45)) = 0; // Client Port (2 bytes) TODO
	  *((ap_uint<16>*) (out_chunk.buff.data+47)) = 0; // Server Port (2 bytes) TODO

	  *((ap_uint<112>*) (out_chunk.buff.data+49)) = 0; // Data

	  // Packet block configuration — 14 data bytes needed
	  out_chunk.type = DATA;
	  out_chunk.data_bytes = PARTIAL_DATA;
	  out_chunk.dbuff_id = header_out.dbuff_id;
	  header_out.data_offset += PARTIAL_DATA;
        } else {
	  out_chunk.type = DATA;
	  out_chunk.data_bytes = ALL_DATA;
	  out_chunk.dbuff_id = header_out.dbuff_id;
	  header_out.data_offset += ALL_DATA;
        }
        break;
      }

      case GRANT: {
	if (header_out.processed_bytes == 0) {
	  // TODO very repetitive

	  // Ethernet header
	  htno_set<6>(*((ap_uint<48>*) (out_chunk.buff.data)), MAC_DST); // TODO MAC Dest (set by phy?)
	  htno_set<6>(*((ap_uint<48>*) (out_chunk.buff.data + 6)), MAC_SRC); // TODO MAC Src (set by phy?)
	  htno_set<2>(*((ap_uint<16>*) (out_chunk.buff.data + 12)), ETHERTYPE_IPV6); // Ethertype

	  // IPv6 header
	  htno_set<4>(*((ap_uint<32>*) (out_chunk.buff.data + 14)), VTF); // Version/Traffic Class/Flow Label
	  htno_set<2>(*((ap_uint<16>*) (out_chunk.buff.data + 18)), header_out.packet_bytes - PREFACE_HEADER); // Payload Length
	  htno_set<1>(*((ap_uint<8>*) (out_chunk.buff.data + 20)), IPPROTO_HOMA); // Next Header
	  htno_set<1>(*((ap_uint<8>*) (out_chunk.buff.data + 21)), HOP_LIMIT); // Hop Limit
	  htno_set<16>(*((ap_uint<128>*) (out_chunk.buff.data + 22)), header_out.saddr); // Sender Address
	  htno_set<16>(*((ap_uint<128>*) (out_chunk.buff.data + 38)), header_out.daddr); // Destination Address

	  // Start of common header
	  htno_set<2>(*((ap_uint<16>*) (out_chunk.buff.data + 54)), header_out.sport); // Sender Port
	  htno_set<2>(*((ap_uint<16>*) (out_chunk.buff.data + 56)), header_out.dport); // Destination Port
	  // Unused

	  // Packet block configuration — no data bytes needed
	  out_chunk.type = GRANT;
	  out_chunk.data_bytes = NO_DATA;
	  header_out.data_offset += NO_DATA;

	} else if (header_out.processed_bytes == 64) {
	  // TODO very repetitive

	  // Rest of common header
	  *((ap_uint<16>*) (out_chunk.buff.data)) = 0; // Unused (2 bytes)
	  htno_set<1>(*((ap_uint<8>*) (out_chunk.buff.data+2)), DOFF);      // doff (4 byte chunks in data header)
	  htno_set<1>(*((ap_uint<8>*) (out_chunk.buff.data+3)), DATA_TYPE); // Type
	  *((ap_uint<16>*) (out_chunk.buff.data+4)) = 0; // Unused (2 bytes)
	  *((ap_uint<16>*) (out_chunk.buff.data+6)) = 0; // Checksum (unused) (2 bytes)
	  *((ap_uint<16>*) (out_chunk.buff.data+8)) = 0; // Unused  (2 bytes)
	  htno_set<8>(*((ap_uint<64>*) (out_chunk.buff.data+10)), header_out.sender_id); // Sender ID

	  // Grant header
	  htno_set<4>(*((ap_uint<32>*) (out_chunk.buff.data+18)), header_out.grant_offset); // Byte offset of grant
	  htno_set<1>(*((ap_uint<8>*) (out_chunk.buff.data+22)), header_out.grant_offset); // TODO Priority

	  // Packet block configuration — 14 data bytes needed
	  out_chunk.type = GRANT;
	  out_chunk.data_bytes = NO_DATA;
	  header_out.data_offset += NO_DATA;
	} 
      }
    }

    header_out.processed_bytes += 64;

    if (header_out.processed_bytes >= header_out.packet_bytes) {
      header_out.valid = 0;
      out_chunk.last = 1;
      doublebuff[r_pkt].valid = 0; // TODO maybe not needed?
      r_pkt++;
    } else {
      out_chunk.last = 0;
    }

    chunk_out_o.write(out_chunk);
  }
}

/**
 * pkt_chunk_ingress() - Take packet chunks from the link, reconstruct packets,
 * write data to a temporary holding buffer
 * @link_ingress - Raw 64B chunks from the link of incoming packets
 * @header_in_o - Outgoing stream for incoming headers to peer map
 * @chunk_in_o - Outgoing stream for storing data until we know
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

  static header_t header_in;
  static in_chunk_t data_block;

  raw_stream_t raw_stream = link_ingress.read();

  //std::cerr << "packet chunk in\n";

  // For every type of homa packet we need to read at least two blocks
  if (header_in.processed_bytes == 0) {
    header_in.processed_bytes += 64;

    ap_uint<16> payload_length;
    ap_uint<128> saddr;
    ap_uint<128> daddr;
    ap_uint<16> sport;
    ap_uint<16> dport;

    htno_set<2>(payload_length, *((ap_uint<16>*) (raw_stream.data.data + 18)));
    htno_set<16>(saddr, *((ap_uint<128>*) (raw_stream.data.data + 22)));
    htno_set<16>(daddr, *((ap_uint<128>*) (raw_stream.data.data + 38)));
    htno_set<2>(sport, *((ap_uint<16>*) (raw_stream.data.data + 54)));
    htno_set<2>(dport, *((ap_uint<16>*) (raw_stream.data.data + 56)));

    header_in.payload_length = payload_length;
    header_in.saddr = saddr;
    header_in.daddr = daddr;
    header_in.sport = sport;
    header_in.dport = dport;
  } else if (header_in.processed_bytes == 64) {
    header_in.processed_bytes += 64;
    ap_uint<8> type;
    htno_set<1>(type, *((ap_uint<8>*) (raw_stream.data.data + 3)));
    header_in.type = (homa_packet_type) (uint8_t) type;  // Packet type

    ap_uint<64> id;
    htno_set<8>(id, *((ap_uint<64>*) (raw_stream.data.data + 10)));
    header_in.sender_id = LOCALIZE_ID(id); // Sender RPC ID

    header_in.valid = 1;

    switch(header_in.type) {
      case GRANT: {
	ap_uint<32> offset;
	ap_uint<8> priority;

	// Grant header
	htno_set<4>(offset, *((ap_uint<32>*) (raw_stream.data.data+18))); // Byte offset of grant
	htno_set<1>(priority, *((ap_uint<8>*) (raw_stream.data.data+22))); // TODO Priority

	header_in.grant_offset = offset;

	break;
      }

      case DATA: {
	ap_uint<32> message_length;
	ap_uint<32> incoming;
	ap_uint<32> data_offset;
	ap_uint<32> segment_length;
	htno_set<4>(message_length, *((ap_uint<32>*) (raw_stream.data.data+18))); // Message Length (entire message)
	htno_set<4>(incoming, *((ap_uint<32>*) (raw_stream.data.data+22)));       // Expected Incoming Bytes
	htno_set<4>(data_offset, *((ap_uint<32>*) (raw_stream.data.data+29)));    // Offset in message of segment
	htno_set<4>(segment_length, *((ap_uint<32>*) (raw_stream.data.data+33))); // Segment Length

	header_in.message_length = message_length;
	header_in.incoming = incoming;
	header_in.data_offset = data_offset;
	header_in.segment_length = segment_length;

	// TODO parse acks

	for (int i = 0; i < PARTIAL_DATA; ++i) {
#pragma HLS unroll
	  data_block.buff.data[i] = raw_stream.data.data[64 - PARTIAL_DATA + i];
	}

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
    data_block.offset = 0;
    header_in.processed_bytes = 0;
  }
}
