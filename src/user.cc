#include "user.hh"
#include "fifo.hh"

/**
 * homa_recvmsg() - Maps completed messages to user level homa receive calls. 
 * @msghdr_recv_i - Incoming recvmsg requests from the user
 * @msghdr_recv_o - Returned recvmsg responsed with DMA offset
 * @header_in_i   - The final header received for a completed message. This
 * indicates the message has been fully received and buffering is complete.
 * TODO Should we for some notification that the message has been fully
 * buffered? If so the user could get the return recvmsg before the data has
 * made it to DMA?
 */
void homa_recvmsg(hls::stream<msghdr_recv_t> & msghdr_recv_i,
		  hls::stream<msghdr_recv_t> & msghdr_recv_o,
		  hls::stream<header_t> & header_in_i) {

    static recv_interest_t recv[MAX_RECV_MATCH];
    static uint32_t recv_head = 0;
  
    static msghdr_recv_t msgs[MAX_RECV_MATCH];
    static uint32_t msgs_head = 0;

    if (!msghdr_recv_i.empty()) {
	msghdr_recv_t msghdr_recv = msghdr_recv_i.read();
      
	msghdr_recv_t match;
	uint32_t match_index = -1;

	for (uint32_t i = 0; i < recv_head; ++i) {
#pragma HLS pipeline II=1
	    // Is there a match
	    if (msgs[i](MSGHDR_SPORT) == msghdr_recv(MSGHDR_SPORT)
		&& msgs[i](MSGHDR_RECV_FLAGS) & msghdr_recv(MSGHDR_RECV_FLAGS) == 1) {
		// Is there an explicit ID match
		if (msgs[i](MSGHDR_RECV_ID) == msghdr(MSGHDR_RECV_ID)) {
		    match = msgs[i];
		    match_index = i;
		    break;
		} else if (match_index == -1 || msgs[i](MSGHDR_IOV_SIZE) < match(MSGHDR_IOV_SIZE)) {
		    match = msgs[i]; 
		    match_index = i;
		} 
	    }
	}

	// No match was found
	if (match_index == -1) {
	    recv_interest_t recv_interst = {msghdr_recv(MSGHDR_SPORT), msghdr_recv(MSGHDR_RECV_FLAGS), msghdr_recv(MSGHDR_RECV_ID)};
	    recv[recv_head] = recv_interest;

	    if (recv_head < MAX_RECV_MATCH) {
		recv_head++;
	    }
	} else {
	    msgs[match_index] = msgs[msgs_head];
	    msghdr_recv_o.write(match);
	    msgs_head--;
	}
    }

    if (!header_in_i.empty()) {

	header_t header_in = header_in_i.read();

	msghdr_recv_t new_msg;

	new_msg(MSGHDR_SADDR)      = header_in.saddr;
	new_msg(MSGHDR_DADDR)      = header_in.daddr;
	new_msg(MSGHDR_SPORT)      = header_in.sport;
	new_msg(MSGHDR_DPORT)      = header_in.dport;
	new_msg(MSGHDR_IOV)        = header_in.dbuff_id;          // TODO
	new_msg(MSGHDR_IOV_SIZE)   = header_in.message_length;
	new_msg(MSGHDR_RECV_ID)    = header_in.local_id; 
	new_msg(MSGHDR_RECV_CC)    = header_in.completion_cookie; // TODO
	new_msg(MSGHDR_RECV_FLAGS) = IS_CLIENT(header_in.id) ? HOMA_RECVMSG_RESPONSE : HOMA_RECVMSG_REQUEST;

	uint32_t match_index = -1;

	for (uint32_t i = 0; i < recv_head; ++i) {
#pragma HLS pipeline II=1
	    // Is there a match
	    if (recv[i].sport == new_msg(MSGHDR_SPORT) && (recv[i].flags & new_msg(MSGHDR_RECV_FLAGS) == 1)) {
		if (recv[i].id == new_msg(MSGHDR_RECV_ID)) {
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
	    msghdr_recv_o.write(new_msg);
	    recv_head--;
	}
    }
}

/**
 * sendmsg() - Assigns a new ID for sendmsg requests that the user can use to
 * later recieve data from this operation. Begins the process of onboarding the
 * new RPC into the homa kernel.
 * @msghdr_send_i - New sendmsg requests from the user. Includes location of
 * message, size of message, ports and addrs.
 * @msghdr_send_o - Result returned to user that includes the ID assigned to
 * the previous sendmsg request
 * @onboard_send_o - Path to prime the system to send the new message
 * TODO needs to return the newly assigned ID to the user
 */
void homa_sendmsg(hls::stream<msghdr_send_t> & msghdr_send_i,
		  hls::stream<msghdr_send_t> & msghdr_send_o,
		  hls::stream<onboard_send_t> & onboard_send_o) {

#pragma HLS pipeline II=1
    // Bisect the ID space?

    // TODO needs to get the new ID and return to user
    // TODO it would be nice if ID construction could be handled here but it is also needed for incoming pktsj?
    msghdr_send_t msghdr_send = msghdr_send_i.read();

    onboard_send_t onboard_send;
    onboard_send.saddr      = msghdr_send(MSGHDR_SADDR);
    onboard_send.daddr      = msghdr_send(MSGHDR_DADDR);
    onboard_send.sport      = msghdr_send(MSGHDR_SPORT);
    onboard_send.dport      = msghdr_send(MSGHDR_DPORT);
    onboard_send.iov        = msghdr_send(MSGHDR_IOV);
    onboard_send.iov_size   = msghdr_send(MSGHDR_IOV_SIZE);
    onboard_send.id         = msghdr_send(MSGHDR_SEND_ID);
    onboard_send.cc         = msghdr_send(MSGHDR_SEND_CC);
    onboard_send.flags      = msghdr_send(MSGHDR_SEND_FLAGS);

    onboard_send_o.write(onboard_send);
}
