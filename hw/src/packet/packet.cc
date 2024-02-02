#include "packet.hh"

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
void network_order(ap_uint<512> & in, ap_uint<512> & out) {
#pragma HLS inline
    for (int i = 0; i < 64; ++i) {
#pragma HLS unroll
	out(512 - (i*8) - 1, 512 - 8 - (i*8)) = in(7 + (i*8), (i*8));
    }
}

/*
 * Construct an outgoing header
 */
/*
void hdr_gen(hls::stream<srpt_queue_entry_t> & header_i,
	     hls::stream<ap_uint<1024>> & header_o,
	     ap_uint<MSGHDR_SEND_SIZE> send_cbs[MAX_RPCS]) {

    // Get RPC state associated with this packet 
    static ap_uint<MSGHDR_SEND_SIZE> cb = send_cbs[sendmsg(SRPT_QUEUE_ENTRY_RPC_ID)];

    ap_uint<1024> header;
	
    // Ethernet header
    header(HDR_IPV6_MAC_DEST)  = MAC_DST;                         // TODO MAC Dest (set by phy?)
    header(HDR_IPV6_MAC_SRC)   = MAC_SRC;                         // TODO MAC Src (set by phy?)
    header(HDR_IPV6_ETHERTYPE) = IPV6_ETHERTYPE;                  // Ethertype
	
    // IPv6 header
    header(HDR_IPV6_VERSION)     = IPV6_VERSION;                  // Version
    header(HDR_IPV6_TRAFFIC)     = IPV6_TRAFFIC;                  // Traffic Class
    header(HDR_IPV6_FLOW)        = IPV6_FLOW;                     // Flow Label
    header(HDR_IPV6_PAYLOAD_LEN) = 0xffffffff;                    // TODO Payload Length 
    header(HDR_IPV6_NEXT_HEADER) = IPPROTO_HOMA;                  // Next Header
    header(HDR_IPV6_HOP_LIMIT)   = IPV6_HOP_LIMIT;                // Hop Limit
    header(HDR_IPV6_SADDR)       = cb(HDR_BLOCK_SADDR);           // Sender Address
    header(HDR_IPV6_DADDR)       = cb(HDR_BLOCK_DADDR);           // Destination Address
	
    // Common header
    header(HDR_HOMA_COMMON_SPORT) = cb(HDR_BLOCK_SPORT);          // Sender Port
    header(HDR_HOMA_COMMON_DPORT) = cb(HDR_BLOCK_DPORT);          // Destination Port
	
    // Unused
    header(HDR_HOMA_COMMON_UNUSED0) = 0;                          // Unused
	
    // Rest of common header
    header(HDR_HOMA_COMMON_UNUSED1)   = 0;                        // Unused (2 bytes)
    header(HDR_HOMA_COMMON_DOFF)      = DOFF;                     // doff (4 byte chunks in data header)
    header(HDR_HOMA_COMMON_TYPE)      = DATA;                     // Type
    header(HDR_HOMA_COMMON_UNUSED3)   = 0;                        // Unused (2 bytes)
    header(HDR_HOMA_COMMON_CHECKSUM)  = 0;                        // Checksum (unused) (2 bytes)
    header(HDR_HOMA_COMMON_UNUSED4)   = 0;                        // Unused  (2 bytes) 
    header(HDR_HOMA_COMMON_SENDER_ID) = cb(HDR_BLOCK_RPC_ID);     // Sender ID
	    
    // Data header
    header(HDR_HOMA_DATA_MSG_LEN)  = cb(HDR_BLOCK_LENGTH);        // Message Length (entire message)
    header(HDR_HOMA_DATA_INCOMING) = 0xffffffff;                  // Incoming TODO
    header(HDR_HOMA_DATA_CUTOFF)   = 0;                           // Cutoff Version (unimplemented) (2 bytes) TODO
    header(HDR_HOMA_DATA_REXMIT)   = 0;                           // Retransmit (unimplemented) (1 byte) TODO
    header(HDR_HOMA_DATA_PAD)      = 0;                           // Pad (1 byte)
	    
    // Data Segment
    header(HDR_HOMA_DATA_OFFSET)   = 0;                           // TODO Offset
    header(HDR_HOMA_DATA_SEG_LEN)  = 1386;                        // TODO 
	    
    // Ack header
    header(HDR_HOMA_ACK_ID)       = 0;                            // Client ID (8 bytes) TODO
    header(HDR_HOMA_ACK_SPORT)    = 0;                            // Client Port (2 bytes) TODO
    header(HDR_HOMA_ACK_DPORT)    = 0;                            // Server Port (2 bytes) TODO

    header(OUT_OF_BAND)           =  sendmsg;

    header_o.write(header);
}
 
void pkt_gen(
    hls::stream<ap_uint<1024>> & header_i,
    hls::stream<ram_cmd_t> & ram_cmd_o,
    hls::stream<ap_axiu<512,0,0,0>> & ram_data_i,
    hls::stream<ram_status_t> & ram_status_i,
    hls::stream<ap_axiu<512, 0, 0, 0>> & link_egress_o) {

#pragma HLS interface mode=ap_ctrl_none port=return
#pragma HLS pipeline II=1
 
    static ap_uint<512> pending_chunks[256];
    static ram_cmd_t    pending_ramread[256];
    static ap_uint<8>   tag;
    static bool         valid;

#pragma HLS interface axis port=header_i
#pragma HLS interface axis port=ram_cmd_o
#pragma HLS interface axis port=ram_data_i
#pragma HLS interface axis port=link_egress_o

    static ap_uint<1024> header;
    static ap_uint<32> processed_bytes;

    if (valid || header_i.read_nb(header)) {
	cb(HDR_BLOCK_LENGTH) - sendmsg(SRPT_QUEUE_ENTRY_REMAINING) + processed_bytes;

	valid = true;
	ram_read_t ram_read;
	switch (processed_bytes) {
	    case 0: {
		ram_read.data(RAMR_CMD_ADDR) = ;
		ram_read.data(RAMR_CMD_LEN)  = 1;

		ram_read.data(RAMR_CMD_TAG) = tag++;

		pending_ramread[ram_read.data(RAMR_CMD_TAG)] = ram_read;
		pending_chunks[ram_read.data(RAMR_CMD_TAG)]  = natural_chunk;

		ram_cmd_o.write(ram_read);
 
		pending_chunks[tag++] = header(511,0);

	    }
	    case 64: {
		pending_chunks[tag++] = header(1023,511);
	    }
	    default:
	}

	processed_bytes += 64;
	ram_cmd_o.write(ram_read);
    }

    if (!ram_status_i.empty() && !ram_data_i.empty()) {
    	ram_status_t ram_status = ram_status_i.read();
    	ap_axiu<512,0,0,0> ram_data = ram_data_i.read();
    	ram_cmd_t ramread = pending_ramread[ram_status.data(RAMR_STATUS_TAG)];
    	// TODO fetch the frame here as well

    	out_chunk.data = ram_data.data;
    	link_egress_o.write(out_chunk);
    }
}
*/
    // static ap_uint<MSGHDR_SEND_SIZE> cb;
    // static srpt_queue_entry_t sendmsg;
    // static ap_uint<32> processed_bytes = 0;
    // static ap_uint<32> data_bytes = 0; 
    // static bool valid = false;
    
    // ap_uint<32> msg_addr = cb(HDR_BLOCK_MSG_ADDR) - sendmsg(SRPT_QUEUE_ENTRY_REMAINING);
    // ap_uint<32> offset = (sendmsg(SRPT_QUEUE_ENTRY_DBUFF_ID) * 16384);
    // ap_uint<32> offset = (cb(HDR_BLOCK_MSG_ADDR) + data_bytes + (sendmsg(SRPT_QUEUE_ENTRY_DBUFF_ID) * 16384));
    // offset(5,0) = 0;
    // ap_uint<32> offset = 0;
    // ap_uint<32> offset = cb(HDR_BLOCK_MSG_ADDR) + data_bytes;
    // ap_uint<32> chunk_offset = (offset % 16384) / 64;
    // ap_uint<32> byte_offset  = offset % 64;
    
    // ap_uint<512> natural_chunk;
    // ap_axiu<512, 0, 0, 0> out_chunk;

    // ram_cmd_t ram_read;


    // if (valid) {
    // 	if (processed_bytes == 0) {
    // 	    cb = send_cbs[sendmsg(SRPT_QUEUE_ENTRY_RPC_ID)];
    // 	
    // 	    // Ethernet header
    // 	    natural_chunk(CHUNK_IPV6_MAC_DEST)  = MAC_DST;            // TODO MAC Dest (set by phy?)
    // 	    natural_chunk(CHUNK_IPV6_MAC_SRC)   = MAC_SRC;            // TODO MAC Src (set by phy?)
    // 	    natural_chunk(CHUNK_IPV6_ETHERTYPE) = IPV6_ETHERTYPE;     // Ethertype
    // 	
    // 	    // IPv6 header
    // 	    // ap_uint<16> payload_length = (header.packet_bytes - PREFACE_HEADER);
    // 	
    // 	    natural_chunk(CHUNK_IPV6_VERSION)     = IPV6_VERSION;     // Version
    // 	    natural_chunk(CHUNK_IPV6_TRAFFIC)     = IPV6_TRAFFIC;     // Traffic Class
    // 	    natural_chunk(CHUNK_IPV6_FLOW)        = IPV6_FLOW;        // Flow Label
    // 	    natural_chunk(CHUNK_IPV6_PAYLOAD_LEN) = 0xffffffff;       // Payload Length TODO
    // 	    natural_chunk(CHUNK_IPV6_NEXT_HEADER) = IPPROTO_HOMA;     // Next Header
    // 	    natural_chunk(CHUNK_IPV6_HOP_LIMIT)   = IPV6_HOP_LIMIT;   // Hop Limit
    // 	    natural_chunk(CHUNK_IPV6_SADDR)       = cb(HDR_BLOCK_SADDR);     // Sender Address
    // 	    natural_chunk(CHUNK_IPV6_DADDR)       = cb(HDR_BLOCK_DADDR);     // Destination Address
    // 	
    // 	    // Start of common header
    // 	    natural_chunk(CHUNK_HOMA_COMMON_SPORT) = cb(HDR_BLOCK_SPORT);    // Sender Port
    // 	    natural_chunk(CHUNK_HOMA_COMMON_DPORT) = cb(HDR_BLOCK_DPORT);    // Destination Port
    // 	
    // 	    // Unused
    // 	    natural_chunk(CHUNK_HOMA_COMMON_UNUSED0) = 0;             // Unused
    // 	
    // 	    processed_bytes += 64;
    // 	
    // 	    network_order(natural_chunk, out_chunk.data);

    // 	    ram_read.data(RAMR_CMD_ADDR) = offset;
    // 	    ram_read.data(RAMR_CMD_LEN)  = 1;
    // 	
    // 	    processed_bytes += 64;

    // 	    ram_read.data(RAMR_CMD_TAG) = tag++;

    // 	    pending_ramread[ram_read.data(RAMR_CMD_TAG)] = ram_read;
    // 	    pending_chunks[ram_read.data(RAMR_CMD_TAG)]  = natural_chunk;

    // 	    ram_cmd_o.write(ram_read);
    // 	
    // 	    // out_chunk.keep = 0xFFFFFFFFFFFFFFFF;
    // 	
    // 	    // link_egress_o.write(out_chunk);
    // 	
    // 	} else {
    // 	    if (processed_bytes == 64) {
    // 		// Rest of common header
    // 		natural_chunk(CHUNK_HOMA_COMMON_UNUSED1)   = 0;                        // Unused (2 bytes)
    // 		natural_chunk(CHUNK_HOMA_COMMON_DOFF)      = DOFF;                     // doff (4 byte chunks in data header)
    // 		natural_chunk(CHUNK_HOMA_COMMON_TYPE)      = DATA;                     // Type
    // 		natural_chunk(CHUNK_HOMA_COMMON_UNUSED3)   = 0;                        // Unused (2 bytes)
    // 		natural_chunk(CHUNK_HOMA_COMMON_CHECKSUM)  = 0;                        // Checksum (unused) (2 bytes)
    // 		natural_chunk(CHUNK_HOMA_COMMON_UNUSED4)   = 0;                        // Unused  (2 bytes) 
    // 		natural_chunk(CHUNK_HOMA_COMMON_SENDER_ID) = cb(HDR_BLOCK_RPC_ID); // Sender ID
    // 	    
    // 		// Data header
    // 		natural_chunk(CHUNK_HOMA_DATA_MSG_LEN)  = 0xffffffff;      // Message Length (entire message) TODO
    // 		natural_chunk(CHUNK_HOMA_DATA_INCOMING) = 0xffffffff;      // Incoming TODO
    // 		natural_chunk(CHUNK_HOMA_DATA_CUTOFF)   = 0;               // Cutoff Version (unimplemented) (2 bytes) TODO
    // 		natural_chunk(CHUNK_HOMA_DATA_REXMIT)   = 0;               // Retransmit (unimplemented) (1 byte) TODO
    // 		natural_chunk(CHUNK_HOMA_DATA_PAD)      = 0;               // Pad (1 byte)
    // 	    
    // 		// Data Segment
    // 		natural_chunk(CHUNK_HOMA_DATA_OFFSET)  = msg_addr;     // Offset
    // 		natural_chunk(CHUNK_HOMA_DATA_SEG_LEN) = 1386;
    // 		// TODO // sendmsg(HDR_BLOCK_LENGTH);  // Segment Length
    // 	    
    // 		// Ack header
    // 		natural_chunk(CHUNK_HOMA_ACK_ID)    = 0;                         // Client ID (8 bytes) TODO
    // 		natural_chunk(CHUNK_HOMA_ACK_SPORT) = 0;                         // Client Port (2 bytes) TODO
    // 		natural_chunk(CHUNK_HOMA_ACK_DPORT) = 0;                         // Server Port (2 bytes) TODO
    // 	    
    // 		// TODO may need byte swap
    // 		// natural_chunk(111, 0) = double_buff(((byte_offset + 14) * 8) - 1, byte_offset * 8);
    // 	    
    // 		data_bytes += 14;

    // 		ram_read.data(RAMR_CMD_ADDR) = offset;
    // 		ram_read.data(RAMR_CMD_LEN)  = 14;
    // 		
    // 		// TODO Store this frame pending return of RAM read
    // 		
    // 	    } else {
    // 		// TODO may need byte swap
    // 		// natural_chunk = double_buff(((byte_offset + 64) * 8) - 1, byte_offset * 8);
    // 	    
    // 		data_bytes += 64;

    // 		ram_read.data(RAMR_CMD_ADDR) = offset;
    // 		ram_read.data(RAMR_CMD_LEN)  = 64;
    // 	    }
    // 	
    // 	    processed_bytes += 64;

    // 	    ram_read.data(RAMR_CMD_TAG) = tag++;
    // 	    pending_ramread[ram_read.data(RAMR_CMD_TAG)] = ram_read;
    // 	    pending_chunks[ram_read.data(RAMR_CMD_TAG)]  = natural_chunk;

    // 	    ram_cmd_o.write(ram_read);
    // 	
    // 	    if (processed_bytes >= 1386) {
    // 		valid = false;
    // 	    
    // 		processed_bytes = 0;
    // 	    
    // 		data_bytes = 0;
    // 		// out_chunk.last = 1;
    // 	    }
    // 	}
    // 	
    // 	// network_order(natural_chunk, out_chunk.data);
    // 	
    // 	// out_chunk.keep = 0xFFFFFFFFFFFFFFFF;
    // 	
    // 	// Dispatch read request
    // 	// Write partial frame 
    // 	// link_egress_o.write(out_chunk);
    // }

    // if (!ram_status_i.empty() && !ram_data_i.empty()) {
    // 	ram_status_t ram_status = ram_status_i.read();
    // 	ap_axiu<512,0,0,0> ram_data = ram_data_i.read();
    // 	ram_cmd_t ramread = pending_ramread[ram_status.data(RAMR_STATUS_TAG)];
    // 	// TODO fetch the frame here as well

    // 	out_chunk.data = ram_data.data;
    // 	link_egress_o.write(out_chunk);
    // }

    // if (!valid && header_out_i.read_nb(sendmsg)) {
    // 	valid = true;
    // }
// }




























void pkt_ctor(ap_uint<HDR_BLOCK_SIZE> send_cbs[MAX_RPCS],
	      hls::stream<ram_cmd_t> & ram_cmd_o,
	      hls::stream<ram_status_t> & ram_status_i,
	      hls::stream<ap_axiu<512,0,0,0>> & ram_data_i,
	      hls::stream<srpt_queue_entry_t> & header_out_i,
	      hls::stream<ap_axiu<512, 0, 0, 0>> & link_egress_o) {

#pragma HLS interface mode=ap_ctrl_none port=return
// #pragma HLS pipeline II=1
 
//    static ap_uint<512> pending_chunks[256];
//    static ram_cmd_t    pending_ramread[256];
    static ap_uint<8>   tag;
    static ap_uint<32> offset;

#pragma HLS interface bram port=send_cbs
#pragma HLS interface axis port=ram_cmd_o
#pragma HLS interface axis port=ram_status_i
#pragma HLS interface axis port=ram_data_i
#pragma HLS interface axis port=header_out_i
#pragma HLS interface axis port=link_egress_o
    static bool pending = false;

    ap_axiu<512, 0, 0, 0> out_chunk;

    static srpt_queue_entry_t sendmsg;
    if (!pending && header_out_i.read_nb(sendmsg)) {
	ram_cmd_t ram_read;

	// for (int i = 0; i < 16; ++i) {
	    ram_read.data(RAMR_CMD_LEN)  = 256;
	    ram_read.data(RAMR_CMD_ADDR) = 8192 - offset*256;
	    ram_read.data(RAMR_CMD_TAG)  = tag++;

	    offset += 1;
	    pending = true;
	    ram_cmd_o.write(ram_read);

	    out_chunk.data = sendmsg;
	    link_egress_o.write(out_chunk);
    } else if (!ram_data_i.empty()) {

	ap_axiu<512,0,0,0> ram_data = ram_data_i.read();

	out_chunk.data = ram_data.data;
	link_egress_o.write(out_chunk);
    } else if (!ram_status_i.empty()) {
	ram_status_t ram_status = ram_status_i.read();
	pending = false;

	out_chunk.data = ram_status.data;
	link_egress_o.write(out_chunk);
    }

/**
 * pkt_builder() - Take in the packet header data and output
 * structured 64 byte chunks of that packet. Chunks requiring message
 * data will be augmented with that data in the next step. 
 * @header_out_i - Header data that should be sent on the link
 * @chunk_out_o  - 64 byte structured packet chunks output to this stream to
 * be augmented with data from the data buffer
 */
//void pkt_ctor(ap_uint<HDR_BLOCK_SIZE> send_cbs[MAX_RPCS],
//	      hls::stream<ram_cmd_t> & ram_cmd_o,
//	      hls::stream<ram_status_t> & ram_status_i,
//	      hls::stream<ap_axiu<512,0,0,0>> & ram_data_i,
//	      hls::stream<srpt_queue_entry_t> & header_out_i,
//	      hls::stream<ap_axiu<512, 0, 0, 0>> & link_egress_o) {
//
//#pragma HLS interface mode=ap_ctrl_none port=return
//#pragma HLS pipeline II=1
// 
//    static ap_uint<512> pending_chunks[256];
//    static ram_cmd_t    pending_ramread[256];
//    static ap_uint<8>   tag;
//
//#pragma HLS interface bram port=send_cbs
//#pragma HLS interface axis port=ram_cmd_o
//#pragma HLS interface axis port=ram_status_i
//#pragma HLS interface axis port=ram_data_i
//#pragma HLS interface axis port=header_out_i
//#pragma HLS interface axis port=link_egress_o
//
//    static ap_uint<HDR_BLOCK_SIZE> cb;
//    static srpt_queue_entry_t sendmsg;
//    static ap_uint<32> processed_bytes = 0;
//    static ap_uint<32> data_bytes = 0; 
//    static bool valid = false;
//    
//    // ap_uint<32> msg_addr = cb(HDR_BLOCK_MSG_ADDR) - sendmsg(SRPT_QUEUE_ENTRY_REMAINING);
//    // ap_uint<32> offset = (sendmsg(SRPT_QUEUE_ENTRY_DBUFF_ID) * 16384);
//    // ap_uint<32> offset = (cb(HDR_BLOCK_MSG_ADDR) + data_bytes + (sendmsg(SRPT_QUEUE_ENTRY_DBUFF_ID) * 16384));
//    // offset(5,0) = 0;
//    // ap_uint<32> offset = msg_addr + processed_bytes;
//    // ap_uint<32> offset = cb(HDR_BLOCK_MSG_ADDR) + data_bytes;
//    // ap_uint<32> chunk_offset = (offset % 16384) / 64;
//    // ap_uint<32> byte_offset  = offset % 64;
//    
//    ap_uint<512> natural_chunk;
//    ap_axiu<512, 0, 0, 0> out_chunk;
//
//    static ap_uint<32> data_offset = 0;
//
//    ram_cmd_t ram_read;
//
//    if (valid) {
//	if (processed_bytes == 0) {
//	    cb = send_cbs[sendmsg(SRPT_QUEUE_ENTRY_RPC_ID)];
//	
//	    // Ethernet header
//	    natural_chunk(CHUNK_IPV6_MAC_DEST)  = MAC_DST;            // TODO MAC Dest (set by phy?)
//	    natural_chunk(CHUNK_IPV6_MAC_SRC)   = MAC_SRC;            // TODO MAC Src (set by phy?)
//	    natural_chunk(CHUNK_IPV6_ETHERTYPE) = IPV6_ETHERTYPE;     // Ethertype
//	
//	    // IPv6 header
//	    // ap_uint<16> payload_length = (header.packet_bytes - PREFACE_HEADER);
//	
//	    natural_chunk(CHUNK_IPV6_VERSION)     = IPV6_VERSION;     // Version
//	    natural_chunk(CHUNK_IPV6_TRAFFIC)     = IPV6_TRAFFIC;     // Traffic Class
//	    natural_chunk(CHUNK_IPV6_FLOW)        = IPV6_FLOW;        // Flow Label
//	    natural_chunk(CHUNK_IPV6_PAYLOAD_LEN) = 0xffffffff;       // Payload Length TODO
//	    natural_chunk(CHUNK_IPV6_NEXT_HEADER) = IPPROTO_HOMA;     // Next Header
//	    natural_chunk(CHUNK_IPV6_HOP_LIMIT)   = IPV6_HOP_LIMIT;   // Hop Limit
//	    natural_chunk(CHUNK_IPV6_SADDR)       = cb(HDR_BLOCK_SADDR);     // Sender Address
//	    natural_chunk(CHUNK_IPV6_DADDR)       = cb(HDR_BLOCK_DADDR);     // Destination Address
//	
//	    // Start of common header
//	    natural_chunk(CHUNK_HOMA_COMMON_SPORT) = cb(HDR_BLOCK_SPORT);    // Sender Port
//	    natural_chunk(CHUNK_HOMA_COMMON_DPORT) = cb(HDR_BLOCK_DPORT);    // Destination Port
//	
//	    // Unused
//	    natural_chunk(CHUNK_HOMA_COMMON_UNUSED0) = 0;             // Unused
//	
//	    processed_bytes += 64;
//	
//	    network_order(natural_chunk, out_chunk.data);
//
//	    ram_read.data(RAMR_CMD_ADDR) = data_offset;// cb(HDR_BLOCK_LENGTH) - sendmsg(SRPT_QUEUE_ENTRY_REMAINING) + processed_bytes;
//	    ram_read.data(RAMR_CMD_LEN)  = 1;
//	
//	    processed_bytes += 64;
//
//	    ram_read.data(RAMR_CMD_TAG) = tag++;
//
//	    pending_ramread[ram_read.data(RAMR_CMD_TAG)] = ram_read;
//	    pending_chunks[ram_read.data(RAMR_CMD_TAG)]  = natural_chunk;
//
//	    ram_cmd_o.write(ram_read);
//	
//	    // out_chunk.keep = 0xFFFFFFFFFFFFFFFF;
//
//
//	    // link_egress_o.write(out_chunk);
//	
//	} else {
//	    if (processed_bytes == 64) {
//		// Rest of common header
//		natural_chunk(CHUNK_HOMA_COMMON_UNUSED1)   = 0;                        // Unused (2 bytes)
//		natural_chunk(CHUNK_HOMA_COMMON_DOFF)      = DOFF;                     // doff (4 byte chunks in data header)
//		natural_chunk(CHUNK_HOMA_COMMON_TYPE)      = DATA;                     // Type
//		natural_chunk(CHUNK_HOMA_COMMON_UNUSED3)   = 0;                        // Unused (2 bytes)
//		natural_chunk(CHUNK_HOMA_COMMON_CHECKSUM)  = 0;                        // Checksum (unused) (2 bytes)
//		natural_chunk(CHUNK_HOMA_COMMON_UNUSED4)   = 0;                        // Unused  (2 bytes) 
//		natural_chunk(CHUNK_HOMA_COMMON_SENDER_ID) = cb(HDR_BLOCK_RPC_ID); // Sender ID
//	    
//		// Data header
//		natural_chunk(CHUNK_HOMA_DATA_MSG_LEN)  = 0xffffffff;      // Message Length (entire message) TODO
//		natural_chunk(CHUNK_HOMA_DATA_INCOMING) = 0xffffffff;      // Incoming TODO
//		natural_chunk(CHUNK_HOMA_DATA_CUTOFF)   = 0;               // Cutoff Version (unimplemented) (2 bytes) TODO
//		natural_chunk(CHUNK_HOMA_DATA_REXMIT)   = 0;               // Retransmit (unimplemented) (1 byte) TODO
//		natural_chunk(CHUNK_HOMA_DATA_PAD)      = 0;               // Pad (1 byte)
//	    
//		// Data Segment
//		natural_chunk(CHUNK_HOMA_DATA_OFFSET)  = 0;     // Offset
//		natural_chunk(CHUNK_HOMA_DATA_SEG_LEN) = 1386;
//		// TODO // sendmsg(HDR_BLOCK_LENGTH);  // Segment Length
//	    
//		// Ack header
//		natural_chunk(CHUNK_HOMA_ACK_ID)    = 0;                         // Client ID (8 bytes) TODO
//		natural_chunk(CHUNK_HOMA_ACK_SPORT) = 0;                         // Client Port (2 bytes) TODO
//		natural_chunk(CHUNK_HOMA_ACK_DPORT) = 0;                         // Server Port (2 bytes) TODO
//	    
//		// TODO may need byte swap
//		// natural_chunk(111, 0) = double_buff(((byte_offset + 14) * 8) - 1, byte_offset * 8);
//	    
//		data_bytes += 14;
//
//		// 	ram_read.data(RAMR_CMD_ADDR) = offset;
//
//		ram_read.data(RAMR_CMD_ADDR) = data_offset; // cb(HDR_BLOCK_LENGTH) - sendmsg(SRPT_QUEUE_ENTRY_REMAINING) + processed_bytes;
//		ram_read.data(RAMR_CMD_LEN)  = 14;
//		
//		// TODO Store this frame pending return of RAM read
//		
//	    } else {
//		// TODO may need byte swap
//		// natural_chunk = double_buff(((byte_offset + 64) * 8) - 1, byte_offset * 8);
//	    
//		data_bytes += 64;
//
//		// ram_read.data(RAMR_CMD_ADDR) = offset;
//		ram_read.data(RAMR_CMD_ADDR) = data_offset; // cb(HDR_BLOCK_LENGTH) - sendmsg(SRPT_QUEUE_ENTRY_REMAINING) + processed_bytes;
//		ram_read.data(RAMR_CMD_LEN)  = 64;
//	    }
//
//	    data_offset += 64;
//	    processed_bytes += 64;
//
//	    ram_read.data(RAMR_CMD_TAG) = tag++;
//	    pending_ramread[ram_read.data(RAMR_CMD_TAG)] = ram_read;
//	    pending_chunks[ram_read.data(RAMR_CMD_TAG)]  = natural_chunk;
//
//	    ram_cmd_o.write(ram_read);
//	
//	    if (processed_bytes >= 1386) {
//		valid = false;
//	    
//		processed_bytes = 0;
//	    
//		data_bytes = 0;
//		// out_chunk.last = 1;
//	    }
//	}
//	
//	// network_order(natural_chunk, out_chunk.data);
//	
//	// out_chunk.keep = 0xFFFFFFFFFFFFFFFF;
//	
//	// Dispatch read request
//	// Write partial frame 
//	// link_egress_o.write(out_chunk);
//    }
//
//    if (!ram_status_i.empty() && !ram_data_i.empty()) {
//	ram_status_t ram_status = ram_status_i.read();
//	ap_axiu<512,0,0,0> ram_data = ram_data_i.read();
//	ram_cmd_t ramread = pending_ramread[ram_status.data(RAMR_STATUS_TAG)];
//	// TODO fetch the frame here as well
//
//	out_chunk.data = ram_data.data;
//	link_egress_o.write(out_chunk);
//    }
//
//    if (!valid && header_out_i.read_nb(sendmsg)) {
//	valid = true;
//    }

//    static dma_r_req_t pending_reqs[256];
//
//    static ap_uint<MSGHDR_SEND_SIZE> cb;
//    static srpt_queue_entry_t sendmsg;
//    static ap_uint<32> processed_bytes = 0;
//    static ap_uint<32> data_bytes = 0; 
//    static bool valid = false;
//
//    ap_uint<32> msg_addr = cb(HDR_BLOCK_MSG_ADDR) - sendmsg(SRPT_QUEUE_ENTRY_REMAINING);
//
//    ap_uint<32> offset = cb(HDR_BLOCK_MSG_ADDR) + data_bytes;
//
//    ap_uint<32> chunk_offset = (offset % 16384) / 64;
//    ap_uint<32> byte_offset  = offset % 64;
//
//    // ap_uint<1024> double_buff = cache[chunk_offset];
//
//    ap_uint<512> natural_chunk;
//    ap_axiu<512, 0, 0, 0> out_chunk;
//    if (!valid && header_out_i.read_nb(sendmsg)) {
//	valid = true;
//
//	cb = send_cbs[sendmsg(SRPT_QUEUE_ENTRY_RPC_ID)];
//
//        // Ethernet header
//	natural_chunk(CHUNK_IPV6_MAC_DEST)  = MAC_DST;            // TODO MAC Dest (set by phy?)
//	natural_chunk(CHUNK_IPV6_MAC_SRC)   = MAC_SRC;            // TODO MAC Src (set by phy?)
//	natural_chunk(CHUNK_IPV6_ETHERTYPE) = IPV6_ETHERTYPE;     // Ethertype
//	
//	// IPv6 header
//	// ap_uint<16> payload_length = (header.packet_bytes - PREFACE_HEADER);
//	
//	natural_chunk(CHUNK_IPV6_VERSION)     = IPV6_VERSION;     // Version
//	natural_chunk(CHUNK_IPV6_TRAFFIC)     = IPV6_TRAFFIC;     // Traffic Class
//	natural_chunk(CHUNK_IPV6_FLOW)        = IPV6_FLOW;        // Flow Label
//	natural_chunk(CHUNK_IPV6_PAYLOAD_LEN) = 0xffffffff;       // Payload Length TODO
//	natural_chunk(CHUNK_IPV6_NEXT_HEADER) = IPPROTO_HOMA;     // Next Header
//	natural_chunk(CHUNK_IPV6_HOP_LIMIT)   = IPV6_HOP_LIMIT;   // Hop Limit
//	natural_chunk(CHUNK_IPV6_SADDR)       = cb(HDR_BLOCK_SADDR);     // Sender Address
//	natural_chunk(CHUNK_IPV6_DADDR)       = cb(HDR_BLOCK_DADDR);     // Destination Address
//	
//	// Start of common header
//	natural_chunk(CHUNK_HOMA_COMMON_SPORT) = cb(HDR_BLOCK_SPORT);    // Sender Port
//	natural_chunk(CHUNK_HOMA_COMMON_DPORT) = cb(HDR_BLOCK_DPORT);    // Destination Port
//	
//	// Unused
//	natural_chunk(CHUNK_HOMA_COMMON_UNUSED0) = 0;             // Unused
//
//	processed_bytes += 64;
//
//	network_order(natural_chunk, out_chunk.data);
//	    
//	out_chunk.keep = 0xFFFFFFFFFFFFFFFF;
//	    
//	link_egress_o.write(out_chunk);
//    } else if (valid) {
//
//	// switch(header.type) {
//	// case DATA: {
//	if (processed_bytes == 64) {
//	    // Rest of common header
//	    natural_chunk(CHUNK_HOMA_COMMON_UNUSED1)   = 0;                        // Unused (2 bytes)
//	    natural_chunk(CHUNK_HOMA_COMMON_DOFF)      = DOFF;                     // doff (4 byte chunks in data header)
//	    natural_chunk(CHUNK_HOMA_COMMON_TYPE)      = DATA;                     // Type
//	    natural_chunk(CHUNK_HOMA_COMMON_UNUSED3)   = 0;                        // Unused (2 bytes)
//	    natural_chunk(CHUNK_HOMA_COMMON_CHECKSUM)  = 0;                        // Checksum (unused) (2 bytes)
//	    natural_chunk(CHUNK_HOMA_COMMON_UNUSED4)   = 0;                        // Unused  (2 bytes) 
//	    natural_chunk(CHUNK_HOMA_COMMON_SENDER_ID) = cb(HDR_BLOCK_RPC_ID); // Sender ID
//
//	    // Data header
//	    natural_chunk(CHUNK_HOMA_DATA_MSG_LEN)  = 0xffffffff;      // Message Length (entire message) TODO
//	    natural_chunk(CHUNK_HOMA_DATA_INCOMING) = 0xffffffff;      // Incoming TODO
//	    natural_chunk(CHUNK_HOMA_DATA_CUTOFF)   = 0;               // Cutoff Version (unimplemented) (2 bytes) TODO
//	    natural_chunk(CHUNK_HOMA_DATA_REXMIT)   = 0;               // Retransmit (unimplemented) (1 byte) TODO
//	    natural_chunk(CHUNK_HOMA_DATA_PAD)      = 0;               // Pad (1 byte)
//
//	    // Data Segment
//	    natural_chunk(CHUNK_HOMA_DATA_OFFSET)  = msg_addr;     // Offset
//	    natural_chunk(CHUNK_HOMA_DATA_SEG_LEN) = 1386;
//		// TODO // sendmsg(HDR_BLOCK_LENGTH);  // Segment Length
//
//	    // Ack header
//	    natural_chunk(CHUNK_HOMA_ACK_ID)    = 0;                         // Client ID (8 bytes) TODO
//	    natural_chunk(CHUNK_HOMA_ACK_SPORT) = 0;                         // Client Port (2 bytes) TODO
//	    natural_chunk(CHUNK_HOMA_ACK_DPORT) = 0;                         // Server Port (2 bytes) TODO
//
//	    // TODO may need byte swap
//	    // natural_chunk(111, 0) = double_buff(((byte_offset + 14) * 8) - 1, byte_offset * 8);
//
//	    data_bytes += 14;
//
//	} else {
//	    // TODO may need byte swap
//	    // natural_chunk = double_buff(((byte_offset + 64) * 8) - 1, byte_offset * 8);
//
//	    data_bytes += 64;
//	}
//
//	processed_bytes += 64;
//
//	if (processed_bytes >= 1386) {
//	    valid = false;
//
//	    processed_bytes = 0;
//
//	    data_bytes = 0;
//	    out_chunk.last = 1;
//	}
//
//	network_order(natural_chunk, out_chunk.data);
//	    
//// 	raw_stream.last = chunk.last_pkt_chunk;
//// 	raw_stream.keep = (0xFFFFFFFFFFFFFFFF >> (64 - chunk.link_bytes));
//// 	link_egress.write(raw_stream);
////      raw_stream.keep = (0xFFFFFFFFFFFFFFFF >> (64 - link_bytes)); ??????
//	    
//	out_chunk.keep = 0xFFFFFFFFFFFFFFFF;
//	    
//	link_egress_o.write(out_chunk);
//    }
}


//	    case GRANT: {
//		if (processed_bytes == 0) {
//		    // TODO this is a little repetetive
//
//		    // Ethernet header
//		    natural_chunk(CHUNK_IPV6_MAC_DEST)  = MAC_DST;        // TODO MAC Dest (set by phy?)
//		    natural_chunk(CHUNK_IPV6_MAC_SRC)   = MAC_SRC;        // TODO MAC Src (set by phy?)
//		    natural_chunk(CHUNK_IPV6_ETHERTYPE) = IPV6_ETHERTYPE; // Ethertype
//
//		    // IPv6 header 
//		    ap_uint<16> payload_length = (header.packet_bytes - PREFACE_HEADER);
//
//		    natural_chunk(CHUNK_IPV6_VERSION)        = IPV6_VERSION;     // Version
//		    natural_chunk(CHUNK_IPV6_TRAFFIC)        = IPV6_TRAFFIC;     // Traffic Class
//		    natural_chunk(CHUNK_IPV6_FLOW)           = IPV6_FLOW;        // Flow Label
//		    natural_chunk(CHUNK_IPV6_PAYLOAD_LEN)    = payload_length;   // Payload Length
//		    natural_chunk(CHUNK_IPV6_NEXT_HEADER)    = IPPROTO_HOMA;     // Next Header
//		    natural_chunk(CHUNK_IPV6_HOP_LIMIT)      = IPV6_HOP_LIMIT;   // Hop Limit
//		    natural_chunk(CHUNK_IPV6_SADDR)          = header.saddr;     // Sender Address
//		    natural_chunk(CHUNK_IPV6_DADDR)          = header.daddr;     // Destination Address
//
//		    // Start of common header
//		    natural_chunk(CHUNK_HOMA_COMMON_SPORT)   = header.sport;     // Sender Port
//		    natural_chunk(CHUNK_HOMA_COMMON_DPORT)   = header.dport;     // Destination Port
//
//		    // Unused
//		    natural_chunk(CHUNK_HOMA_COMMON_UNUSED0) = 0;                // Unused
//
//		    // Packet block configuration — no data bytes needed
//		    out_chunk.type       = GRANT;
//		    out_chunk.data_bytes = 0;
//		    out_chunk.link_bytes = 64;
//		} else if (processed_bytes == 64) {
//		    // Rest of common header
//		    natural_chunk(CHUNK_HOMA_COMMON_UNUSED1)   = 0;                // Unused (2 bytes)
//		    natural_chunk(CHUNK_HOMA_COMMON_DOFF)      = DOFF;             // doff (4 byte chunks in data header)
//		    natural_chunk(CHUNK_HOMA_COMMON_TYPE)      = GRANT;            // Type
//		    natural_chunk(CHUNK_HOMA_COMMON_UNUSED3)   = 0;                // Unused (2 bytes)
//		    natural_chunk(CHUNK_HOMA_COMMON_CHECKSUM)  = 0;                // Checksum (unused) (2 bytes)
//		    natural_chunk(CHUNK_HOMA_COMMON_UNUSED4)   = 0;                // Unused  (2 bytes) 
//		    natural_chunk(CHUNK_HOMA_COMMON_SENDER_ID) = header.id;        // Sender ID
//
//		    // Grant header
//		    natural_chunk(CHUNK_HOMA_GRANT_OFFSET)   = header.grant_offset; // Byte offset of grant
//		    natural_chunk(CHUNK_HOMA_GRANT_PRIORITY) = 0;                       // Priority TODO
//
//		    // Packet block configuration — no data bytes needed
//		    out_chunk.type       = GRANT;
//		    out_chunk.data_bytes = 0;
//		    out_chunk.link_bytes = 54;
//		} 
//		break;
//	    }


/**
 * pkt_chunk_egress() - Take care of final configuration before packet
 * chunks are placed on the link. Set TKEEP and TLAST bits for AXI Stream
 * protocol.
 * TODO this can maybe be absorbed by data buffer core, it just makes less
 * sense for the "databuffer" to be writing to the link
 * @out_chunk_i - Chunks to be output onto the link
 * @link_egress - Outgoign AXI Stream to the link
 */
// void pkt_chunk_egress(hls::stream<h2c_chunk_t> & out_chunk_i,
// 		      hls::stream<raw_stream_t> & link_egress) {
// #pragma HLS pipeline
//     if (!out_chunk_i.empty()) {
// 
// 	h2c_chunk_t chunk = out_chunk_i.read();
// 	raw_stream_t raw_stream;
// 
// 	raw_stream.data = chunk.data;
// 	raw_stream.last = chunk.last_pkt_chunk;
// 	raw_stream.keep = (0xFFFFFFFFFFFFFFFF >> (64 - chunk.link_bytes));
// 	link_egress.write(raw_stream);
//     }
// }

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
void pkt_dtor(hls::stream<ap_axiu<512, 0, 0, 0>> & link_ingress,
	      ap_uint<MSGHDR_SEND_SIZE> recv_cbs[MAX_RPCS],
	      hls::stream<dma_w_req_t> & dma_w_req_o,
	      ap_uint<MSGHDR_RECV_SIZE> & recvmsg_o) {

#pragma HLS interface mode=ap_ctrl_none port=return

#pragma HLS pipeline II=1

#pragma HLS interface bram port=recv_cbs
#pragma HLS interface axis port=link_ingress
#pragma HLS interface axis port=dma_w_req_o
#pragma HLS interface axis port=recvmsg_o

    // static header_t header_in;
    static ap_uint<32> processed_bytes = 0;

    if (!link_ingress.empty()) {
	ap_axiu<512, 0, 0, 0> raw_stream = link_ingress.read();
	
	ap_uint<512> natural_chunk;

	network_order(raw_stream.data, natural_chunk);

	dma_w_req_t dma_w_req;

	// For every type of homa packet we need to read at least two blocks
	if (processed_bytes == 0) {

	    // header_in.payload_length = natural_chunk(CHUNK_IPV6_PAYLOAD_LEN);

	    // header_in.saddr          = natural_chunk(CHUNK_IPV6_SADDR);
	    // header_in.daddr          = natural_chunk(CHUNK_IPV6_DADDR);
	    // header_in.sport          = natural_chunk(CHUNK_HOMA_COMMON_SPORT);
	    // header_in.dport          = natural_chunk(CHUNK_HOMA_COMMON_DPORT);

	} else if (processed_bytes == 64) {

	    //  header_in.type = natural_chunk(CHUNK_HOMA_COMMON_TYPE);      // Packet type
	    //  header_in.id   = natural_chunk(CHUNK_HOMA_COMMON_SENDER_ID); // Sender RPC ID
	    //  header_in.id   = LOCALIZE_ID(header_in.id);

	    // switch(header_in.type) {
	    // case GRANT: {
	    //     // Grant header
	    //     header_in.grant_offset = natural_chunk(CHUNK_HOMA_GRANT_OFFSET);   // Byte offset of grant
	    //     header_in.priority     = natural_chunk(CHUNK_HOMA_GRANT_PRIORITY); // TODO priority

	    //     break;
	    // }

	    // case DATA: {
	    //  header_in.message_length = natural_chunk(CHUNK_HOMA_DATA_MSG_LEN);  // Message Length (entire message)
	    //  header_in.incoming       = natural_chunk(CHUNK_HOMA_DATA_INCOMING); // Expected Incoming Bytes
	    //  header_in.data_offset    = natural_chunk(CHUNK_HOMA_DATA_OFFSET);   // Offset in message of segment
	    //  header_in.segment_length = natural_chunk(CHUNK_HOMA_DATA_SEG_LEN);  // Segment Length

	    dma_w_req(DMA_W_REQ_DATA) = raw_stream.data(511, 512-14*8);
	    // TODO parse acks

	    // data_block.data(14 * 8, 0) = raw_stream.data(511, 512-14*8);

// 		    ap_uint<8> pop  = 0;
// 		    for (int i = 0; i < 64; i++) {
// #pragma HLS unroll
// 		      pop += ((raw_stream.keep >> i) & 1);
// 		    }

	    // data_block.width  = pop - 50;
	    // data_block.last   = raw_stream.last;

	    dma_w_req_o.write(dma_w_req);

	    // break;
	    // }
	    // }

	    // header_in_o.write(header_in);
	} else {
	    // data_block.data  = raw_stream.data;

// 	    ap_uint<8> pop  = 0;
// 	    for (int i = 0; i < 64; i++) {
// #pragma HLS unroll
// 	      pop += ((raw_stream.keep >> i) & 1);
// 	    }

	    // data_block.width = pop;
	    // data_block.last  = raw_stream.last;
	    dma_w_req(DMA_W_REQ_DATA) = raw_stream.data;

	    dma_w_req_o.write(dma_w_req);
	}

	processed_bytes += 64;

	if (raw_stream.last) {
	    processed_bytes = 0;
	}
    }
}
