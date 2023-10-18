#include <time.h>
#include <stdlib.h>

#include "homa.hh"
#include "rpcmgmt.hh"

hls::stream<msghdr_send_t> msghdr_send_i;
hls::stream<msghdr_send_t> msghdr_send_o;
hls::stream<srpt_queue_entry_t> data_queue_o;
hls::stream<srpt_queue_entry_t> fetch_queue_o;
hls::stream<header_t> h2c_header_i; 
hls::stream<header_t> h2c_header_o;
hls::stream<header_t> c2h_header_i;
hls::stream<header_t> c2h_header_o;
hls::stream<srpt_grant_new_t> grant_srpt_o;
hls::stream<srpt_queue_entry_t> data_srpt_o;
hls::stream<srpt_queue_entry_t> dbuff_notif_i;
hls::stream<srpt_queue_entry_t> dbuff_notif_o;
hls::stream<port_to_phys_t> c2h_port_to_phys_i;
hls::stream<dma_w_req_t> dma_w_req_i;
hls::stream<dma_w_req_t> dma_w_req_o;
hls::stream<port_to_phys_t> h2c_port_to_phys_i;
hls::stream<srpt_queue_entry_t> dma_r_req_i;
hls::stream<dma_r_req_t> dma_r_req_o;
hls::stream<ap_uint<8>> h2c_pkt_log_o;
hls::stream<ap_uint<8>> c2h_pkt_log_o;

void cycle() {
    rpc_state(
	      msghdr_send_i, 
	      msghdr_send_o,
	      data_queue_o,
	      fetch_queue_o,
	      h2c_header_i, 
	      h2c_header_o,
	      c2h_header_i,
	      c2h_header_o,
	      grant_srpt_o,
	      data_srpt_o,
	      dbuff_notif_i,
	      dbuff_notif_o,
	      c2h_port_to_phys_i,
	      dma_w_req_i,
	      dma_w_req_o,
	      h2c_port_to_phys_i,
	      dma_r_req_i,
	      dma_r_req_o,
	      h2c_pkt_log_o,
	      c2h_pkt_log_o
	);
}

void test_tashtable() {
    // TODO need to test overflow

    cycle();

    c2h_header_i.write(c2h_header_in);

    cycle();

    c2h_header_in.sport = 0x0;
    c2h_header_in.dport = 0x0;

    c2h_header_i.write(c2h_header_in);

    cycle();

    c2h_header_o.read(c2h_header_out);

    std::cerr << c2h_header_out.local_id << std::endl;

    c2h_header_o.read(c2h_header_out);

    std::cerr << c2h_header_out.local_id << std::endl;

    c2h_header_o.read(c2h_header_out);

    std::cerr << c2h_header_out.local_id << std::endl;
}

int main() {
    srand(time(NULL));
    std::cerr << "****************************** rpc_state() -- hashmap tester ******************************" << std::endl;

    std::cerr << "****************************** rpc_state() -- new client ******************************" << std::endl;

    header_t c2h_header_in;
    c2h_header_in.type = DATA;

    for (int i = 0; i < MAX_RPCS; ++i) {
	c2h_header_in.id    = rand();
	c2h_header_in.saddr = rand();
	c2h_header_in.daddr = rand();
	c2h_header_in.sport = rand();
	c2h_header_in.dport = rand();

	c2h_header_i.write(c2h_header_in);

	header_t c2h_header_out;
    }


    //    homa_rpc_t homa_rpc;
    //    
    //    homa_rpc.saddr("DCBAFEDCBAFEDCBADCBAFEDCBAFEDCBA", 16);
    //    homa_rpc.daddr("ABCDEFABCDEFABCDABCDEFABCDEFABCD", 16);
    //    homa_rpc.sport       = 0xDEAD;
    //    homa_rpc.dport       = 0xBEEF;
    //    homa_rpc.id          = 2;
    //    homa_rpc.buff_addr   = 0;
    //    homa_rpc.buff_size   = 512;
    //    homa_rpc.cc("DCBAFEDCBAFEDCBA", 16);
    //    homa_rpc.local_id    = 2;
    //    homa_rpc.h2c_buff_id = 1;
    //
    //    onboard_send_i.write(homa_rpc);
    //
    //    cycle();
    //
    //    if (!data_queue_o.empty()) {
    //	std::cerr << "FAILED: Missing creation of data srpt queue entry" << std::endl;
    //	return 1;
    //    }
    //
    //    // TODO break this up
    //    /* The data SRPT queue relies on the RPC ID, DBUFF ID, REMAINING, DBUFFERED */
    //    srpt_queue_entry_t data_queue = data_queue_o.read();
    //    if (data_queue(SRPT_QUEUE_ENTRY_RPC_ID) == homa_rpc.local_id) {
    //	std::cerr << "FAILED: Incorrect data srpt queue entry RPC ID" << std::endl;
    //	return 1;
    //    }
    //
    //    if (data_queue(SRPT_QUEUE_ENTRY_RPC_ID) == homa_rpc.local_id) {
    //	std::cerr << "FAILED: Incorrect data srpt queue entry RPC ID" << std::endl;
    //	return 1;
    //    }
    //
    //    if (!fetch_queue_o.empty()) {
    //	std::cerr << "FAILED: Missing creation of data fetch queue entry" << std::endl;
    //	return 1;
    //    }

    std::cerr << "****************************** rpc_state() tester end ******************************" << std::endl;

    return 1;
}
