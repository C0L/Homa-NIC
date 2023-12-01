/* Soft-CPU memory space
 * (0x00000000 - 0x????????) - Soft-CPU Text
 * (0x???????? - 0x????????) - Soft-CPU Data
 * (0x???????? - 0x????????) - Soft-CPU BSS
 * (0x???????? - 0x????????) - Soft-CPU Stack
 * (0x???????? - 0x????????) - Soft-CPU x Host Shared x DMA Controller
 * ......
 */

// TODO the kernel module can get the offsets from the object file?


__attribute__((section(".dma_addrs"))) host_addr_t c2h_port_to_msgbuff[MAX_PORTS];  // Port -> large c2h buffer space 
__attribute__((section(".dma_addrs"))) msg_addr_t  c2h_rpc_to_offset[MAX_RPCS];     // RPC -> offset in that buffer space
__attribute__((section(".dma_addrs"))) ap_uint<7>  c2h_buff_ids[MAX_PORTS];         // Next availible offset in buffer space


// TODO to now be handled by the host
// port_to_phys_t new_h2c_port_to_msgbuff;
// if (h2c_port_to_msgbuff_i.read_nb(new_h2c_port_to_msgbuff)) {
//     h2c_port_to_msgbuff[new_h2c_port_to_msgbuff(PORT_TO_PHYS_PORT)] = new_h2c_port_to_msgbuff(PORT_TO_PHYS_ADDR);
// }
// 
// port_to_phys_t new_c2h_port_to_msgbuff;
// if (c2h_port_to_msgbuff_i.read_nb(new_c2h_port_to_msgbuff)) {
//     c2h_port_to_msgbuff[new_c2h_port_to_msgbuff(PORT_TO_PHYS_PORT)] = new_c2h_port_to_msgbuff(PORT_TO_PHYS_ADDR);
// }
// 
// port_to_phys_t new_c2h_port_to_metadata;
// if (c2h_port_to_metadata_i.read_nb(new_c2h_port_to_metadata)) {
//     c2h_port_to_metadata[new_c2h_port_to_metadata(PORT_TO_PHYS_PORT)] = new_c2h_port_to_metadata(PORT_TO_PHYS_ADDR);
// }



msghdr_send_t msghdr_send;
dbuff_id_t dbuff_id;
if (sendmsg_i.read_nb(msghdr_send)) {
    // Copy sendmsg data to a homa rpc structure
    homa_rpc_t rpc(msghdr_send.data);

    // Allocate space in the data buffer for this RPC h2c data
    rpc.h2c_buff_id = msg_cache_ids.pop();

    // Is this a request or a response
    if (rpc.local_id == 0) { 
	local_id_t new_client = (2 * client_ids.pop());

	std::cerr << "NEW CLIENT: " << new_client << std::endl;

	// The network ID and local ID will be the same
	rpc.local_id = new_client;
	rpc.id       = new_client;
    }

    msghdr_send.data(MSGHDR_SEND_ID) = rpc.id;

    // Store the state associated with this data
    send_rpcs[rpc.local_id] = rpc;

    h2c_rpc_to_offset[rpc.local_id] = (rpc.buff_addr << 1) | 1; // TODO can this get removed?

    // TODO use get function from homa_rpc

    if (rpc.buff_size != 0) {
	srpt_queue_entry_t srpt_data_in = 0;
	srpt_data_in(SRPT_QUEUE_ENTRY_RPC_ID)    = rpc.local_id;
	srpt_data_in(SRPT_QUEUE_ENTRY_DBUFF_ID)  = rpc.h2c_buff_id;
	srpt_data_in(SRPT_QUEUE_ENTRY_REMAINING) = rpc.buff_size;
	srpt_data_in(SRPT_QUEUE_ENTRY_DBUFFERED) = rpc.buff_size;
	// TODO would be cleaner if we could just set this to RTT_BYTES
	srpt_data_in(SRPT_QUEUE_ENTRY_GRANTED)   = rpc.buff_size - ((((ap_uint<32>) RTT_BYTES) > rpc.buff_size)
								    ? rpc.buff_size : ((ap_uint<32>) RTT_BYTES));
	srpt_data_in(SRPT_QUEUE_ENTRY_PRIORITY)  = SRPT_ACTIVE;

	// Insert this entry into the SRPT data queue
	data_queue_o.write(srpt_data_in);
	
	srpt_queue_entry_t srpt_fetch_in = 0;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_RPC_ID)    = rpc.local_id;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_DBUFF_ID)  = rpc.h2c_buff_id;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_REMAINING) = rpc.buff_size;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_DBUFFERED) = 0;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_GRANTED)   = 0;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_PRIORITY)  = SRPT_ACTIVE;
	
	// Insert this entry into the SRPT fetch queue
	fetch_queue_o.write(srpt_fetch_in);
    }
    
    /* Instruct the user that the sendmsg request is active */
    dma_w_req_t msghdr_resp;
    msghdr_resp.data   = msghdr_send.data;
    msghdr_resp.offset = c2h_port_to_metadata[msghdr_send.data(MSGHDR_SPORT)] + (msghdr_send.data(MSGHDR_RETURN) * 64);
    msghdr_resp.strobe = 64;
    sendmsg_dma_o.write(msghdr_resp);

} else if (free_dbuff_i.read_nb(dbuff_id)) {
    msg_cache_ids.push(dbuff_id);
}





// TODO rename
void c2h_metadata(
    hls::stream<msghdr_recv_t> & msghdr_recv_i,
    hls::stream<ap_uint<MSGHDR_RECV_SIZE>> & msghdr_dma_o,
    hls::stream<header_t> & complete_msgs_i
    ) {

// TODO should assign these based on active peers
    static ap_uint<MSGHDR_RECV_SIZE> buffered_complete[MAX_PORTS][MAX_RECV_MATCH];
    static ap_uint<MSGHDR_RECV_SIZE> buffered_pending[MAX_PORTS][MAX_RECV_MATCH];

    static ap_uint<MAX_RECV_LOG2> complete_head[MAX_PORTS];
    static ap_uint<MAX_RECV_LOG2> pending_head[MAX_PORTS];

#pragma HLS dependence variable=buffered_complete inter WAR false
#pragma HLS dependence variable=buffered_complete inter RAW false
#pragma HLS dependence variable=buffered_pending inter WAR false
#pragma HLS dependence variable=buffered_pending inter RAW false
#pragma HLS dependence variable=buffered_pending inter WAR false
#pragma HLS dependence variable=buffered_pending inter RAW false
#pragma HLS dependence variable=complete_head inter WAR false
#pragma HLS dependence variable=complete_head inter RAW false
#pragma HLS dependence variable=pending_head inter WAR false
#pragma HLS dependence variable=pending_head inter RAW false

    static ap_uint<MSGHDR_RECV_SIZE> search_complete      = 0;
    static ap_uint<MSGHDR_RECV_SIZE> search_pending       = 0;
    static ap_uint<MSGHDR_RECV_SIZE> search_result_msghdr = 0;
    static ap_uint<MSGHDR_RECV_SIZE> search_result_index  = 0;
    static ap_uint<MAX_RECV_LOG2>    search_index         = 0;

    header_t complete;
    msghdr_recv_t pending;

    // TODO replace this with stalling CAM
    if (search_complete != 0) {
	ap_uint<MSGHDR_RECV_SIZE> candidate = buffered_pending[search_complete(MSGHDR_SPORT)][search_index];

	std::cerr << "SEARCH INDEX " << search_index << std::endl;

    	if (candidate != 0) {
            // Is this candidate a match and we don't already have an ID match 
    	    if (candidate(MSGHDR_SADDR) == search_complete(MSGHDR_SADDR) &&
    		candidate(MSGHDR_DADDR) == search_complete(MSGHDR_DADDR) &&
    		candidate(MSGHDR_SPORT) == search_complete(MSGHDR_SPORT) &&
    		candidate(MSGHDR_DPORT) == search_complete(MSGHDR_DPORT) &&
    		search_result_msghdr(MSGHDR_RECV_ID) != search_complete(MSGHDR_RECV_ID)) {

    		switch(candidate(MSGHDR_RECV_FLAGS)) {
    		    case HOMA_RECVMSG_REQUEST:
    			if (search_complete(MSGHDR_RECV_FLAGS) == HOMA_RECVMSG_REQUEST) {
    			    search_result_msghdr = candidate;
    			    search_result_index  = search_index;
    			}
    			break;
    		    case HOMA_RECVMSG_RESPONSE:
    			if (search_complete(MSGHDR_RECV_FLAGS) == HOMA_RECVMSG_RESPONSE) {
    			    search_result_msghdr = candidate;
    			    search_result_index  = search_index;
    			}
    			break;
    		    case HOMA_RECVMSG_ALL:
    			search_result_msghdr = candidate;
    			search_result_index  = search_index;
    			break;
    		}
    	    }
    	    search_index++;
    	} else {
    	    if (search_result_msghdr != 0) {
		ap_uint<MSGHDR_RECV_SIZE> msghdr_resp;
    		msghdr_resp = search_result_msghdr;
    		msghdr_resp(MSGHDR_RECV_ID) = search_complete(MSGHDR_RECV_ID);
    		msghdr_resp(MSGHDR_RECV_CC) = search_complete(MSGHDR_RECV_CC);

		// std::cerr << "c2h complete port to meta " << c2h_port_to_metadata[search_result_msghdr(MSGHDR_DPORT)] << std::endl;
		// std::cerr << "c2h complete msg return " << search_result_msghdr(MSGHDR_RETURN) * 64 << std::endl;

		msghdr_dma_o.write(msghdr_resp);

    		buffered_pending[search_complete(MSGHDR_SPORT)][search_result_index] = 0;
    		pending_head[search_complete(MSGHDR_SPORT)]--;
    	    } else {
    		buffered_pending[search_complete(MSGHDR_SPORT)][pending_head[search_complete(MSGHDR_SPORT)]] = search_complete;
    		complete_head[search_complete(MSGHDR_SPORT)]++;
    	    }

    	    search_complete      = 0;
    	    search_pending       = 0;
    	    search_result_msghdr = 0;
    	    search_index         = 0;
    	}
    } else if (search_pending != 0) { // TODO should not compare the entire thing
    	ap_uint<MSGHDR_RECV_SIZE> candidate = buffered_complete[search_pending(MSGHDR_SPORT)][search_index];

    	if (candidate != 0) {
    	    // Is this candidate a match and we don't already have an ID match 
    	    if (candidate(MSGHDR_SADDR) == search_pending(MSGHDR_SADDR) &&
    		candidate(MSGHDR_DADDR) == search_pending(MSGHDR_DADDR) &&
    		candidate(MSGHDR_SPORT) == search_pending(MSGHDR_SPORT) &&
    		candidate(MSGHDR_DPORT) == search_pending(MSGHDR_DPORT) &&
    		search_result_msghdr(MSGHDR_RECV_ID) != search_pending(MSGHDR_RECV_ID)) {
    		switch(candidate(MSGHDR_RECV_FLAGS)) {
    		    case HOMA_RECVMSG_REQUEST:
    			if (search_pending(MSGHDR_RECV_FLAGS) == HOMA_RECVMSG_REQUEST) {
    			    search_result_msghdr = candidate;
    			    search_result_index  = search_index;
    			}
    			break;
    		    case HOMA_RECVMSG_RESPONSE:
    			if (search_pending(MSGHDR_RECV_FLAGS) == HOMA_RECVMSG_RESPONSE) {
    			    search_result_msghdr = candidate;
    			    search_result_index  = search_index;
    			}
    			break;
    		    case HOMA_RECVMSG_ALL:
    			search_result_msghdr = candidate;
			search_result_index  = search_index;
    		}
    	    }
	    search_index++;
    	} else {
    	    if (search_result_msghdr != 0) {
		ap_uint<MSGHDR_RECV_SIZE> msghdr_resp;

    		msghdr_resp = search_result_msghdr;

		msghdr_dma_o.write(msghdr_resp);

		// std::cerr << "c2h pending port to meta " << c2h_port_to_metadata[search_result_msghdr(MSGHDR_DPORT)] << std::endl;
		// std::cerr << "c2h pending msg return " << search_result_msghdr(MSGHDR_RETURN) * 64 << std::endl;

    		buffered_complete[search_pending(MSGHDR_SPORT)][search_result_index] = 0;
    		complete_head[search_complete(MSGHDR_SPORT)]--; // TODO This is wrong

    	    } else {
    		buffered_pending[search_pending(MSGHDR_SPORT)][pending_head[search_pending(MSGHDR_SPORT)]] = search_pending;
    		pending_head[search_pending(MSGHDR_SPORT)]++; // TODO This is wrong
    	    }

    	    search_complete = 0;
    	    search_pending  = 0;
    	    search_result_msghdr = 0;
    	    search_index    = 0;
    	}
    } else if (complete_msgs_i.read_nb(complete)) {
	// TODO swap this
	search_complete(MSGHDR_SADDR)      = complete.saddr;
	search_complete(MSGHDR_DADDR)      = complete.daddr;
	search_complete(MSGHDR_SPORT)      = complete.sport;
	search_complete(MSGHDR_DPORT)      = complete.dport;
	search_complete(MSGHDR_BUFF_ADDR)  = complete.host_addr;
	search_complete(MSGHDR_RETURN)     = complete.host_addr;
	search_complete(MSGHDR_BUFF_SIZE)  = complete.message_length;
	search_complete(MSGHDR_RECV_ID)    = complete.local_id;
	std::cerr << "COMPLETE MESSAGE ID " <<  complete.id << std::endl;
	search_complete(MSGHDR_RECV_CC)    = complete.id;
	search_complete(MSGHDR_RECV_FLAGS) = (IS_CLIENT(complete.local_id)) ? HOMA_RECVMSG_RESPONSE : HOMA_RECVMSG_REQUEST;
    } else if (msghdr_recv_i.read_nb(pending)) {
	search_pending = pending.data;
    }
}



// volatile int test = 99;
volatile int test = 99;
__attribute__((naked, noreturn)) void main(){
    test = 0xdeadbeef;
}
