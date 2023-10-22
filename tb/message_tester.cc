#include <sys/mman.h>
#include <unistd.h>
#include <fstream>
#include <fcntl.h>

#include <chrono>
#include <thread>

#include "homa.hh"

using namespace std;

hls::stream<raw_stream_t>       link_ingress_i;
hls::stream<raw_stream_t>       link_egress_o;
hls::stream<msghdr_send_t>      sendmsg_i;
hls::stream<am_cmd_t>           w_cmd_queue_o;
hls::stream<ap_axiu<512,0,0,0>> w_data_queue_o;
hls::stream<am_status_t>        w_status_queue_i;
hls::stream<am_cmd_t>           r_cmd_queue_o;
hls::stream<ap_axiu<512,0,0,0>> r_data_queue_i;
hls::stream<am_status_t>        r_status_queue_i;
hls::stream<port_to_phys_t>     h2c_port_to_msgbuff_i;
hls::stream<port_to_phys_t>     c2h_port_to_msgbuff_i;
hls::stream<port_to_phys_t>     c2h_port_to_metasend_i;
hls::stream<port_to_phys_t>     c2h_port_to_metarecv_i;
hls::stream<ap_uint<8>>         log_control_i;
hls::stream<log_entry_t>        log_out_o;

ap_uint<128> saddr("DCBAFEDCBAFEDCBADCBAFEDCBAFEDCBA", 16);
ap_uint<128> daddr("ABCDEFABCDEFABCDABCDEFABCDEFABCD", 16);

uint16_t sport = 1;
uint16_t dport = 1;

ap_uint<64> c2h_phys = 0xDEADBEEF;
ap_uint<64> h2c_phys = 0xBEEFDEAD;

port_to_phys_t c2h_msgbuff_map;
port_to_phys_t h2c_msgbuff_map;

port_to_phys_t c2h_metasend_map;
port_to_phys_t c2h_metarecv_map;

char * h2c_msgbuff;
char * c2h_msgbuff;

char * c2h_metasend;
char * c2h_metarecv;

uint64_t sendhead;
uint64_t recvhead;

void process();
ap_uint<MSGHDR_SEND_SIZE> sendmsg(ap_uint<32> size, ap_uint<64> id);
ap_uint<MSGHDR_RECV_SIZE> recvmsg();
void init();

ap_uint<MSGHDR_SEND_SIZE> sendmsg(ap_uint<32> size, ap_uint<64> id) {
    msghdr_send_t sendmsg_req;

    sendmsg_req.data(MSGHDR_SADDR)     = saddr;
    sendmsg_req.data(MSGHDR_DADDR)     = daddr;
    sendmsg_req.data(MSGHDR_SPORT)     = sport;
    sendmsg_req.data(MSGHDR_DPORT)     = dport;
    sendmsg_req.data(MSGHDR_BUFF_ADDR) = 0;
    sendmsg_req.data(MSGHDR_BUFF_SIZE) = size;

    sendmsg_req.data(MSGHDR_SEND_ID)   = id;
    sendmsg_req.data(MSGHDR_SEND_CC)   = 99;

    sendmsg_i.write(sendmsg_req);

    while(sendhead == *((uint64_t*) c2h_metasend)) process();

    ap_uint<MSGHDR_SEND_SIZE> sendmsg_resp;

    // If we just rolled over
    if (*((uint64_t*) c2h_metasend) == 64) {
	sendmsg_resp = *((ap_uint<MSGHDR_SEND_SIZE>*) (c2h_metasend + 16384-64));
    } else {
	sendmsg_resp = *((ap_uint<MSGHDR_SEND_SIZE>*) (c2h_metasend + sendhead));
    }

    sendhead = *((uint64_t*) c2h_metasend);

    return sendmsg_resp;
}

ap_uint<MSGHDR_RECV_SIZE> recvmsg() {

    std::cerr << "STALLING FOR RECVMSG" << std::endl;
    while (recvhead == *((uint64_t*) c2h_metarecv)) process();

    ap_uint<512> recvmsg_resp;

    // If we just rolled over
    if (*((uint64_t*) c2h_metarecv) == 64) {
	recvmsg_resp = *((ap_uint<512>*) (c2h_metarecv + 16384-64));
    } else {
	recvmsg_resp = *((ap_uint<512>*) (c2h_metarecv + recvhead));
    }

    recvhead = *((uint64_t*) c2h_metarecv);

    return recvmsg_resp;
}

// short messages fail

void init() {
    h2c_msgbuff = (char*) malloc(16384 * sizeof(char));
    c2h_msgbuff = (char*) malloc(16384 * sizeof(char));

    c2h_metasend = (char*) malloc(16384 * sizeof(char));
    c2h_metarecv = (char*) malloc(16384 * sizeof(char));

    *((uint64_t*) c2h_metasend) = 64;
    *((uint64_t*) c2h_metarecv) = 64;

    sendhead = 64;
    recvhead = 64;

    c2h_msgbuff_map(PORT_TO_PHYS_ADDR) = (uint64_t) c2h_msgbuff;
    c2h_msgbuff_map(PORT_TO_PHYS_PORT) = sport;

    h2c_msgbuff_map(PORT_TO_PHYS_ADDR) = (uint64_t) h2c_msgbuff;
    h2c_msgbuff_map(PORT_TO_PHYS_PORT) = sport;

    c2h_metasend_map(PORT_TO_PHYS_ADDR) = (uint64_t) c2h_metasend;
    c2h_metasend_map(PORT_TO_PHYS_PORT) = sport;

    c2h_metarecv_map(PORT_TO_PHYS_ADDR) = (uint64_t) c2h_metarecv;
    c2h_metarecv_map(PORT_TO_PHYS_PORT) = sport;

    h2c_port_to_msgbuff_i.write(h2c_msgbuff_map);
    c2h_port_to_msgbuff_i.write(c2h_msgbuff_map);

    c2h_port_to_metasend_i.write(c2h_metasend_map);
    c2h_port_to_metarecv_i.write(c2h_metarecv_map);
}

/* Simulate Network and PCIe */
void process() {
    if (!link_egress_o.empty()) {
	link_ingress_i.write(link_egress_o.read());
    }
    
    if (!r_cmd_queue_o.empty()) {
	am_cmd_t am_cmd = r_cmd_queue_o.read();
	ap_axiu<512,0,0,0> read_data;
	am_status_t am_status;
	
	memcpy((char*) &read_data, (char*) ((uintptr_t) am_cmd.data(AM_CMD_SADDR)), am_cmd.data(AM_CMD_BTT) * sizeof(char));
	r_data_queue_i.write(read_data);
	r_status_queue_i.write(am_status);
    }
    
    if (!w_cmd_queue_o.empty()) {
	
	am_cmd_t am_cmd = w_cmd_queue_o.read();
	ap_axiu<512,0,0,0> write_data = w_data_queue_o.read();
	
	am_status_t am_status;

	memcpy((char*) ((uintptr_t) am_cmd.data(AM_CMD_SADDR)), (char*) ((uintptr_t) &write_data.data), am_cmd.data(AM_CMD_BTT) * sizeof(char));
	w_status_queue_i.write(am_status);
    }
}

void random_rpc() {

    char pattern[5] = "\xDE\xAD\xBE\xEF";

    for (int i = 0; i < (16384/4); ++i) memcpy(h2c_msgbuff + (i*4), &pattern, 4);
    std::cerr << "ZEROING: " << ((uint64_t) c2h_msgbuff) << std::endl;
    memset(c2h_msgbuff, 0, 16384);

    ap_uint<32> size = 6000;
    // ap_uint<32> size = rand() % 4000;

    std::cerr << "size: " << size << std::endl;

    ap_uint<MSGHDR_SEND_SIZE> send = sendmsg(size, 0); // Request

    std::cerr << "SENDMSG ID " << send(MSGHDR_SEND_ID) << std::endl;

    ap_uint<MSGHDR_RECV_SIZE> recv = recvmsg(); // Receipt

    while (memcmp(h2c_msgbuff, c2h_msgbuff, size) != 0) process();

    std::cerr << "RECVMSG ID " << recv(MSGHDR_RECV_ID) << std::endl;

    send = sendmsg(size, recv(MSGHDR_RECV_ID));

    std::cerr << "SENDMSG ID " << send(MSGHDR_SEND_ID) << std::endl;

    recv = recvmsg(); // Receipt

    std::cerr << "RECVMSG ID " << recv(MSGHDR_RECV_ID) << std::endl;


}

int main(int argc, char **argv) {

    srand(time(NULL));

    homa(sendmsg_i,
	 w_cmd_queue_o, w_data_queue_o, w_status_queue_i,
	 r_cmd_queue_o, r_data_queue_i, r_status_queue_i,
	 link_ingress_i, link_egress_o,
	 h2c_port_to_msgbuff_i, c2h_port_to_msgbuff_i,
	 c2h_port_to_metasend_i, c2h_port_to_metarecv_i,
	 log_control_i,
	 log_out_o
	);

    init();

    random_rpc();
    // for (int i = 0; i < 1000; ++i) random_rpc();

    free(h2c_msgbuff);
    free(c2h_msgbuff);
    free(c2h_metasend);
    free(c2h_metarecv);

    return 0;
}
