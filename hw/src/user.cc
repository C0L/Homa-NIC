#include "user.hh"
#include "fifo.hh"
#include "stack.hh"
#include "homa.hh"

#include <iostream>
#include <fstream>

using namespace std;

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
