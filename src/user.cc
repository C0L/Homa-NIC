#include "user.hh"
#include "fifo.hh"
#include "stack.hh"
#include "homa.hh"

#include <iostream>
#include <fstream>

using namespace std;

/**
 * homa_recvmsg() - Maps completed messages to user level homa receive calls. 
 * @msghdr_recv_i - Incoming recvmsg requests from the user
 * @msghdr_recv_o - Returned recvmsg responsed with DMA offset
 * @header_in_i   - The final header received for a completed message. This
 * indicates the message has been fully received and buffering is complete.
 */
void homa_recvmsg(hls::stream<msghdr_recv_t> & msghdr_recv_i,
		  hls::stream<msghdr_recv_t> & msghdr_recv_o,
		  hls::stream<header_t> & header_in_i) {

    static int recv_head = 0;
    static recv_interest_t recv[MAX_RECV_MATCH];
  
    static msghdr_recv_t msgs[MAX_RECV_MATCH];
    static int msgs_head = 0;

    msghdr_recv_t msghdr_recv;
    header_t header_in;

    if (msghdr_recv_i.read_nb(msghdr_recv)) {
    	// msghdr_recv_t match;
    	int match_index = -1;

    	//for (int i = 0; i < msgs_head; ++i) {
    	//    // Is there a match
    	//    if (msgs[i](MSGHDR_SPORT) == msghdr_recv(MSGHDR_SPORT)
    	//	&& msgs[i](MSGHDR_RECV_FLAGS) & msghdr_recv(MSGHDR_RECV_FLAGS) == 1) {
    	//	// Is there an explicit ID match
    	//	if (msgs[i](MSGHDR_RECV_ID) == msghdr_recv(MSGHDR_RECV_ID)) {
    	//	    match = msgs[i];
    	//	    match_index = i;
    	//	    break;
    	//	} else if (match_index == -1 || msgs[i](MSGHDR_IOV_SIZE) < match(MSGHDR_IOV_SIZE)) {
    	//	    match = msgs[i]; 
    	//	    match_index = i;
    	//	} 
    	//    }
    	//}

    	// No match was found
    	if (match_index == -1) {
    	    recv_interest_t recv_interest = {msghdr_recv.data(MSGHDR_SPORT), msghdr_recv.data(MSGHDR_RECV_FLAGS), msghdr_recv.data(MSGHDR_RECV_ID)};
    	    recv[recv_head] = recv_interest;

    	    if (recv_head < MAX_RECV_MATCH) {
    		recv_head++;
    	    }
    	}
    	//else {
    	//    msgs[match_index] = msgs[msgs_head];
    	//    msghdr_recv_o.write(match);
    	//    msgs_head--;
    	//}
    	// }
    } 

    // TODO read the recvmsg in as the last job?
    
    if (header_in_i.read_nb(header_in)) {
	msghdr_recv_t new_msg;

	new_msg.data(MSGHDR_SADDR)      = header_in.saddr;
	new_msg.data(MSGHDR_DADDR)      = header_in.daddr;
	new_msg.data(MSGHDR_SPORT)      = header_in.sport;
	new_msg.data(MSGHDR_DPORT)      = header_in.dport;
	new_msg.data(MSGHDR_BUFF_ADDR)  = header_in.host_addr;
	new_msg.data(MSGHDR_BUFF_SIZE)  = header_in.message_length;
	new_msg.data(MSGHDR_RECV_ID)    = header_in.local_id; 
	new_msg.data(MSGHDR_RECV_CC)    = header_in.completion_cookie; // TODO
	new_msg.data(MSGHDR_RECV_FLAGS) = IS_CLIENT(header_in.id) ? HOMA_RECVMSG_RESPONSE : HOMA_RECVMSG_REQUEST;

	int match_index = -1;

	for (int i = 0; i < recv_head; ++i) {
// #pragma HLS pipeline II=1
	    // Is there a match
	    if (recv[i].sport == new_msg.data(MSGHDR_SPORT) && (recv[i].flags & new_msg.data(MSGHDR_RECV_FLAGS) == 1)) {
		if (recv[i].id == new_msg.data(MSGHDR_RECV_ID)) {
		    match_index = i;
		    break;
		} else if (match_index == -1) {
		    match_index = i;
		} 
	    }
	}

	// No match was found
	if (match_index == -1) {
	    msgs[msgs_head] = new_msg;

	    if (msgs_head < MAX_RECV_MATCH) { 
		msgs_head++;
	    }
	} else {
	    recv[match_index] = recv[recv_head];
	    new_msg.last = 1;
	    msghdr_recv_o.write(new_msg);
	    recv_head--;
	}
    }
}
