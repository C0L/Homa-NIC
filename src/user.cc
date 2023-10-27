#include "user.hh"
#include "fifo.hh"
#include "stack.hh"
#include "homa.hh"

#include <iostream>
#include <fstream>

using namespace std;

void c2h_metadata(
    hls::stream<msghdr_send_t> & sendmsg_i,
    hls::stream<dma_w_req_t> & sendmsg_o,
    hls::stream<homa_rpc_t> & new_rpc_o,
    hls::stream<msghdr_recv_t> & msghdr_recv_i,
    hls::stream<header_t> & complete_msgs_i,
    hls::stream<dma_w_req_t> & msghdr_recv_o,
    hls::stream<port_to_phys_t> & c2h_port_to_metadata_i,
    hls::stream<local_id_t> & new_client_i,
    hls::stream<dbuff_id_t> & new_dbuff_i
    ) {

    /* Port to metadata mapping DMA */
    // TODO should this be here?
    static host_addr_t c2h_port_to_metadata[MAX_PORTS]; // Port -> metadata buffer

//     #pragma HLS pipeline II=1

    msghdr_send_t msghdr_send;
    if (sendmsg_i.read_nb(msghdr_send)) {
	homa_rpc_t rpc;

	if (msghdr_send.data(MSGHDR_SEND_ID) == 0) {
	    msghdr_send.data(MSGHDR_SEND_ID) = new_client_i.read();
	    rpc.id                           = msghdr_send.data(MSGHDR_SEND_ID);
	}

	rpc.local_id = msghdr_send.data(MSGHDR_SEND_ID);

	local_id_t id = msghdr_send.data(MSGHDR_SEND_ID);

	rpc.saddr       = msghdr_send.data(MSGHDR_SADDR);
	rpc.daddr       = msghdr_send.data(MSGHDR_DADDR);
	rpc.sport       = msghdr_send.data(MSGHDR_SPORT);
	rpc.dport       = msghdr_send.data(MSGHDR_DPORT);
	rpc.buff_addr   = msghdr_send.data(MSGHDR_BUFF_ADDR);
	rpc.buff_size   = msghdr_send.data(MSGHDR_BUFF_SIZE);
	rpc.cc          = msghdr_send.data(MSGHDR_SEND_CC);
	rpc.h2c_buff_id = new_dbuff_i.read();

	std::cerr << "READ NEW DBUFF\n";

	/* Instruct the user that the sendmsg request is active */
	dma_w_req_t msghdr_resp;
	msghdr_resp.data   = msghdr_send.data;
	msghdr_resp.offset = c2h_port_to_metadata[msghdr_send.data(MSGHDR_SPORT)] + (msghdr_send.data(MSGHDR_RETURN) * 64);
	msghdr_resp.strobe = 64;
	sendmsg_o.write(msghdr_resp);

	/* Begin evaluating this request */
	new_rpc_o.write(rpc);

	std::cerr << "user.cc completed onboarding" << std::endl;
    }

    port_to_phys_t new_c2h_port_to_metadata;
    if (c2h_port_to_metadata_i.read_nb(new_c2h_port_to_metadata)) {
	c2h_port_to_metadata[new_c2h_port_to_metadata(PORT_TO_PHYS_PORT)] = new_c2h_port_to_metadata(PORT_TO_PHYS_ADDR);
    }

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
#pragma HLS bind_storage variable=c2h_port_to_metadata type=RAM_1WNR

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
    		dma_w_req_t msghdr_resp;

    		msghdr_resp.data = search_result_msghdr;
    		msghdr_resp.data(MSGHDR_RECV_ID) = search_complete(MSGHDR_RECV_ID);
    		msghdr_resp.offset = c2h_port_to_metadata[search_result_msghdr(MSGHDR_DPORT)] + (search_result_msghdr(MSGHDR_RETURN) * 64);

		std::cerr << "c2h complete port to meta " << c2h_port_to_metadata[search_result_msghdr(MSGHDR_DPORT)] << std::endl;
		std::cerr << "c2h complete msg return " << search_result_msghdr(MSGHDR_RETURN) * 64 << std::endl;

    		msghdr_resp.strobe = 64;
    		msghdr_recv_o.write(msghdr_resp);

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
    		
    		dma_w_req_t msghdr_resp;
    		msghdr_resp.data = search_result_msghdr;
    		msghdr_resp.offset = c2h_port_to_metadata[search_result_msghdr(MSGHDR_SPORT)] + (search_result_msghdr(MSGHDR_RETURN) * 64);
    		msghdr_resp.strobe = 64;
    		msghdr_recv_o.write(msghdr_resp);

		std::cerr << "c2h pending port to meta " << c2h_port_to_metadata[search_result_msghdr(MSGHDR_DPORT)] << std::endl;
		std::cerr << "c2h pending msg return " << search_result_msghdr(MSGHDR_RETURN) * 64 << std::endl;

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
	search_complete(MSGHDR_RECV_CC)    = complete.completion_cookie;
	search_complete(MSGHDR_RECV_FLAGS) = (IS_CLIENT(complete.local_id)) ? HOMA_RECVMSG_RESPONSE : HOMA_RECVMSG_REQUEST;
    } else if (msghdr_recv_i.read_nb(pending)) {
	search_pending = pending.data;
    }
}
