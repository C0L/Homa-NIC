#include "user.hh"
#include "fifo.hh"
#include "stack.hh"
#include "homa.hh"

#include <iostream>
#include <fstream>

using namespace std;

// No buffering of complete messages
// Assume the recv from host has arrived before the complete message
// Assign 256 slots to each port: pointer + count

// Trying to match a complete and buffered message to its socket listener
// The process has a set of slots in its metadata buffer
// These slots are subdivided among threads
// Just create a FIFO for each port?

void c2h_metadata(
    hls::stream<msghdr_recv_t> & msghdr_recv_i,
    hls::stream<ap_uint<MSGHDR_RECV_SIZE>> & msghdr_dma_o,
    hls::stream<header_t> & complete_msgs_i
    ) {

    static ap_uint<12> listening[MAX_PORTS][1024];
    static ap_uint<8> heads[MAX_PORTS];

// #pragma HLS dependence variable=buffered intra WAR false
// #pragma HLS dependence variable=buffered intra RAW false
// #pragma HLS dependence variable=heads intra WAR false
// #pragma HLS dependence variable=heads intra RAW false
// #pragma HLS dependence variable=listening intra WAR false
// #pragma HLS dependence variable=listening intra RAW false

// #pragma HLS array_partition variable=buffered type=block factor=256

#pragma HLS pipeline II=3
    msghdr_recv_t pending;
    header_t header_complete;
    if (complete_msgs_i.read_nb(header_complete)) {
	ap_uint<MSGHDR_RECV_SIZE> complete;

 	complete(MSGHDR_SADDR)      = header_complete.saddr;
 	complete(MSGHDR_DADDR)      = header_complete.daddr;
 	complete(MSGHDR_SPORT)      = header_complete.sport;
 	complete(MSGHDR_DPORT)      = header_complete.dport;
 	complete(MSGHDR_BUFF_ADDR)  = header_complete.host_addr;
 	complete(MSGHDR_RETURN)     = header_complete.host_addr;
 	complete(MSGHDR_BUFF_SIZE)  = header_complete.message_length;
 	complete(MSGHDR_RECV_ID)    = header_complete.local_id;
 	complete(MSGHDR_RECV_CC)    = header_complete.id;
 	complete(MSGHDR_RECV_FLAGS) = (IS_CLIENT(header_complete.local_id)) ? HOMA_RECVMSG_RESPONSE : HOMA_RECVMSG_REQUEST;

	std::cerr << "Complete message at port " << complete(MSGHDR_DPORT) << std::endl;

	ap_uint<8> head = heads[complete(MSGHDR_DPORT)];
	ap_uint<12> listener = listening[complete(MSGHDR_DPORT)][head-1];

	std::cerr << "got head " << head << std::endl;

	if (head != 0) {
	    complete(MSGHDR_RETURN) = listener;
	    heads[complete(MSGHDR_DPORT)] = head - 1;
	    msghdr_dma_o.write(complete);
	}
    } else if (msghdr_recv_i.read_nb(pending)) {
	std::cerr << "NEW ENTRY AT PORT " << pending.data(MSGHDR_RETURN) << std::endl;
	ap_uint<8> head = head[pending.data(MSGHDR_DPORT)];
	listening[pending.data(MSGHDR_DPORT)][head] = pending.data(MSGHDR_RETURN);
	heads[pending.data(MSGHDR_DPORT)] = head + 1;
    }
}
