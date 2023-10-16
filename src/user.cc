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

/**
 * sendmsg() - Assigns a new ID for sendmsg requests that the user can use to
 * later recieve data from this operation. Begins the process of onboarding the
 * new RPC into the homa kernel.
 * @msghdr_send_i - New sendmsg requests from the user. Includes location of
 * message, size of message, ports and addrs.
 * @msghdr_send_o - Result returned to user that includes the ID assigned to
 * the previous sendmsg request
 * @onboard_send_o - Path to prime the system to send the new message
 */
// void homa_sendmsg(hls::stream<msghdr_send_t> & msghdr_send_i,
// 		  hls::stream<msghdr_send_t> & msghdr_send_o,
// 		  hls::stream<homa_rpc_t> & onboard_send_o) {
// 
// #pragma HLS pipeline II=1
// 
//     static stack_t<local_id_t, MAX_RPCS> client_ids(STACK_EVEN);
// 
//     /* Eventually we want the SRPT data core to deallocate a buffer
//      * when it becomes the 64th or 128th most desirable RPC. If the
//      * dbuff IDs were hard coded into the entries in the queue then this
//      * would be very challenging, so instead they are managed from here.
//      * The deallocation process within the SRPT queue works by just
//      * setting the number of dbuffered bytes to zero when an entry
//      * exists the active set.
//      *
//      * The top 64 entries of the SRPT queue should be assigned an ID
//      * in the databuffer. They can be created at the time the entry is
//      * added to the queue. As another check in the swap operation, an
//      * entry can steal another entries ID.
//      */
//     static stack_t<local_id_t, NUM_EGRESS_BUFFS> msg_cache_ids(STACK_ALL);
//     msghdr_send_t msghdr_send;
// 
//     msghdr_send_i.read(msghdr_send);
// 
//     // TODO this shuffling is very unfortunate
//     homa_rpc_t onboard_send;
//     onboard_send.saddr          = msghdr_send.data(MSGHDR_SADDR);
//     onboard_send.daddr          = msghdr_send.data(MSGHDR_DADDR);
//     onboard_send.sport          = msghdr_send.data(MSGHDR_SPORT);
//     onboard_send.dport          = msghdr_send.data(MSGHDR_DPORT);
//     onboard_send.buff_addr      = msghdr_send.data(MSGHDR_BUFF_ADDR);
//     onboard_send.buff_size      = msghdr_send.data(MSGHDR_BUFF_SIZE);
//     onboard_send.cc             = msghdr_send.data(MSGHDR_SEND_CC);
//     onboard_send.h2c_buff_id    = msg_cache_ids.pop();
// 
//     /* If the caller provided an ID of 0 this is a request message and we
//      * need to generate a new local ID. Otherwise, this is a response
//      * message and the ID is already valid in homa_rpc buffer
//      */
//     if (msghdr_send.data(MSGHDR_SEND_ID) == 0) {
// 	// Generate a new local ID, and set the RPC ID to be that
// 	onboard_send.local_id            = client_ids.pop();
// 	onboard_send.id                  = onboard_send.local_id;
// 	msghdr_send.data(MSGHDR_SEND_ID) = onboard_send.id;
//     }
// 
//     msghdr_send.last = 1;
//     msghdr_send_o.write(msghdr_send);
// 
//     onboard_send_o.write(onboard_send);
// }


/* A sendmsg request causes a buffer to be selected in the hosts mem */
void h2c_address_map(
    hls::stream<port_to_phys_t> & h2c_port_to_phys_i,
    hls::stream<homa_rpc_t> & sendmsg_i,
    hls::stream<homa_rpc_t> & sendmsg_o,
    hls::stream<srpt_queue_entry_t> & dma_r_req_i,
    hls::stream<dma_r_req_t> & dma_r_req_o
    ) {
    static host_addr_t h2c_port_to_phys[MAX_PORTS];
    static msg_addr_t  h2c_rpc_to_offset[MAX_RPCS];

    // HACK: This state should not be here
    static ap_uint<16> h2c_rpc_to_port[MAX_RPCS];
    static ap_uint<32> h2c_rpc_to_size[MAX_RPCS];

    // TODO dma reqs
    srpt_queue_entry_t dma_fetch;
    if (dma_r_req_i.read_nb(dma_fetch)) {
	// Get the physical address of this ports entire buffer

	msg_addr_t msg_addr   = h2c_rpc_to_offset[dma_fetch(SRPT_QUEUE_ENTRY_RPC_ID)];
	host_addr_t phys_addr = h2c_port_to_phys[h2c_rpc_to_port[dma_fetch(SRPT_QUEUE_ENTRY_RPC_ID)]];

	dma_r_req_t dma_r_req;
	dma_r_req(SRPT_QUEUE_ENTRY_SIZE-1, 0) = dma_fetch;
	dma_r_req(DMA_R_REQ_MSG_LEN) = h2c_rpc_to_size[dma_fetch(SRPT_QUEUE_ENTRY_RPC_ID)];

	msg_addr >>= 1;
	dma_r_req(DMA_R_REQ_HOST_ADDR) = (h2c_rpc_to_size[dma_fetch(SRPT_QUEUE_ENTRY_RPC_ID)] - dma_fetch(SRPT_QUEUE_ENTRY_REMAINING)) + (phys_addr + msg_addr);

	dma_r_req_o.write(dma_r_req);
    }
	
    port_to_phys_t new_h2c_port_to_phys;
    if (h2c_port_to_phys_i.read_nb(new_h2c_port_to_phys)) {
	h2c_port_to_phys[new_h2c_port_to_phys(PORT_TO_PHYS_PORT)] = new_h2c_port_to_phys(PORT_TO_PHYS_ADDR);
    }

    homa_rpc_t sendmsg;
    if (sendmsg_i.read_nb(sendmsg)) {
	h2c_rpc_to_offset[sendmsg.local_id] = (sendmsg.buff_addr << 1) | 1;
	// HACK: This state should not be here
	h2c_rpc_to_port[sendmsg.local_id] = sendmsg.sport;
	h2c_rpc_to_size[sendmsg.local_id] = sendmsg.buff_size;

	sendmsg_o.write(sendmsg);
    }
}

/* A new header causes a buffer to be selected in the hosts mem */
void c2h_address_map(
    hls::stream<port_to_phys_t> & c2h_port_to_phys_i,
    hls::stream<header_t> & c2h_header_i,
    hls::stream<header_t> & c2h_header_o,
    hls::stream<dma_w_req_t> & dma_w_req_i,
    hls::stream<dma_w_req_t> & dma_w_req_o
    ) {

    static host_addr_t c2h_port_to_phys[MAX_PORTS];
    static msg_addr_t  c2h_rpc_to_offset[MAX_RPCS];
    static ap_uint<7>  c2h_buff_ids[MAX_PORTS];

    dma_w_req_t dma_w_req;
    if (dma_w_req_i.read_nb(dma_w_req)) {
	// Get the physical address of this ports entire buffer
	host_addr_t phys_addr = c2h_port_to_phys[dma_w_req.port];
	msg_addr_t msg_addr   = c2h_rpc_to_offset[dma_w_req.rpc_id];

	msg_addr >>= 1;
	dma_w_req.offset += phys_addr + msg_addr;

	dma_w_req_o.write(dma_w_req);
    }
	
    port_to_phys_t new_c2h_port_to_phys;
    if (c2h_port_to_phys_i.read_nb(new_c2h_port_to_phys)) {
	c2h_port_to_phys[new_c2h_port_to_phys(PORT_TO_PHYS_PORT)] = new_c2h_port_to_phys(PORT_TO_PHYS_ADDR);
    }

    header_t c2h_header;
    if (c2h_header_i.read_nb(c2h_header)) {
	if ((c2h_header.packetmap & PMAP_INIT) == PMAP_INIT) {
	    msg_addr_t msg_addr = ((c2h_buff_ids[c2h_header.sport] * HOMA_MAX_MESSAGE_LENGTH) << 1);
	    c2h_rpc_to_offset[c2h_header.local_id] = msg_addr;
	    c2h_buff_ids[c2h_header.sport]++;
	}
	    
	c2h_header_o.write(c2h_header);
    }
}
