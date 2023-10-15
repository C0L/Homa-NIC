#include "homa.hh"

hls::stream<homa_rpc_t> sendmsg_i;
hls::stream<srpt_queue_entry_t> data_entry_o;
hls::stream<srpt_queue_entry_t> fetch_entry_o;
hls::stream<header_t> h2c_header_i;
hls::stream<header_t> h2c_header_o;
hls::stream<header_t> c2h_header_i;
hls::stream<header_t> c2h_header_o;
hls::stream<srpt_grant_new_t> grant_update_o;
hls::stream<srpt_queue_entry_t> data_update_o;
hls::stream<ap_uint<8>> h2c_pkt_log_o;
hls::stream<ap_uint<8>> c2h_pkt_log_o;

void cycle() {
    rpc_state(
	sendmsg_i,
	data_queue_o,
	fetch_queue_o,
	h2c_header_i, 
	h2c_header_o,
	c2h_header_i,
	c2h_header_o,
	grant_update_o,
	data_update_o,
	h2c_pkt_log_o,
	c2h_pkt_log_o
	);
}

void main() {
    std::cerr << "****************************** rpc_state() tester start ******************************" << endl;
    std::cerr << "****************************** rpc_state() -- new client ******************************" << endl;

    homa_rpc_t homa_rpc;
    
    homa_rpc.saddr("DCBAFEDCBAFEDCBADCBAFEDCBAFEDCBA", 16);
    homa_rpc.daddr("ABCDEFABCDEFABCDABCDEFABCDEFABCD", 16);
    homa_rpc.sport       = 0xDEAD;
    homa_rpc.dport       = 0xBEEF;
    homa_rpc.id          = 2;
    homa_rpc.buff_addr   = 0;
    homa_rpc.buff_size   = 512;
    homa_rpc.cc("DCBAFEDCBAFEDCBA", 16);
    homa_rpc.local_id    = 2;
    homa_rpc.h2c_buff_id = 1;

    onboard_send_i.write(homa_rpc);

    cycle();

    if (!data_queue_o.empty()) {
	std::cerr << "FAILED: Missing creation of data srpt queue entry" << std::endl;
	return 1;
    }

    // TODO break this up
    /* The data SRPT queue relies on the RPC ID, DBUFF ID, REMAINING, DBUFFERED */
    srpt_queue_entry_t data_queue = data_queue_o.read();
    if (data_queue(SRPT_QUEUE_ENTRY_RPC_ID) == homa_rpc.local_id) {
	std::cerr << "FAILED: Incorrect data srpt queue entry RPC ID" << std::endl;
	return 1;
    }

    if (data_queue(SRPT_QUEUE_ENTRY_RPC_ID) == homa_rpc.local_id) {
	std::cerr << "FAILED: Incorrect data srpt queue entry RPC ID" << std::endl;
	return 1;
    }

    if (!fetch_queue_o.empty()) {
	std::cerr << "FAILED: Missing creation of data fetch queue entry" << std::endl;
	return 1;
    }

    std::cerr << "****************************** rpc_state() tester end ******************************" << endl;
}
