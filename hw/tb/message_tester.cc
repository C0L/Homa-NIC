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
hls::stream<msghdr_send_t>      recvmsg_i;
hls::stream<am_cmd_t>           w_cmd_queue_o;
hls::stream<ap_axiu<512,0,0,0>> w_data_queue_o;
hls::stream<am_status_t>        w_status_queue_i;
hls::stream<am_cmd_t>           r_cmd_queue_o;
hls::stream<ap_axiu<512,0,0,0>> r_data_queue_i;
hls::stream<am_status_t>        r_status_queue_i;
hls::stream<port_to_phys_t>     h2c_port_to_msgbuff_i;
hls::stream<port_to_phys_t>     c2h_port_to_msgbuff_i;
hls::stream<port_to_phys_t>     c2h_port_to_metadata_i;
hls::stream<ap_uint<512>>       log_control_i;
hls::stream<log_entry_t>        log_out_o;

ap_uint<128> A_saddr("DCBAFEDCBAFEDCBADCBAFEDCBAFEDCBA", 16);
ap_uint<128> A_daddr("ABCDEFABCDEFABCDABCDEFABCDEFABCD", 16);

ap_uint<128> B_daddr("DCBAFEDCBAFEDCBADCBAFEDCBAFEDCBA", 16);
ap_uint<128> B_saddr("ABCDEFABCDEFABCDABCDEFABCDEFABCD", 16);

uint16_t A_sport = 1;
uint16_t A_dport = 2;
 
uint16_t B_sport = 2;
uint16_t B_dport = 1;

// ap_uint<128> A_saddr("DCBAFEDCBAFEDCBADCBAFEDCBAFEDCBA", 16);
// ap_uint<128> A_daddr("ABCDEFABCDEFABCDABCDEFABCDEFABCD", 16);
// 
// ap_uint<128> B_saddr("DCBAFEDCBAFEDCBADCBAFEDCBAFEDCBA", 16);
// ap_uint<128> B_daddr("ABCDEFABCDEFABCDABCDEFABCDEFABCD", 16);
// 
// uint16_t A_sport = 1;
// uint16_t A_dport = 1;
// 
// uint16_t B_sport = 1;
// uint16_t B_dport = 1;

port_to_phys_t A_c2h_msgbuff_map;
port_to_phys_t A_h2c_msgbuff_map;

port_to_phys_t B_c2h_msgbuff_map;
port_to_phys_t B_h2c_msgbuff_map;

port_to_phys_t A_c2h_metadata_map;
port_to_phys_t B_c2h_metadata_map;

char * A_h2c_msgbuff;
char * A_c2h_msgbuff;
char * A_c2h_metadata;

char * B_h2c_msgbuff;
char * B_c2h_msgbuff;
char * B_c2h_metadata;

struct c_msghdr_send_t {
    char saddr[16];
    char daddr[16];
    uint16_t sport;
    uint16_t dport;
    uint64_t buff_addr;
    uint32_t metadata;
    uint64_t id;
    uint64_t cc;
}__attribute__((packed));


void ipv6_to_str(char * str, char * s6_addr) {
   sprintf(str, "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x\0",
                 (int)s6_addr[0],  (int)s6_addr[1],
                 (int)s6_addr[2],  (int)s6_addr[3],
                 (int)s6_addr[4],  (int)s6_addr[5],
                 (int)s6_addr[6],  (int)s6_addr[7],
                 (int)s6_addr[8],  (int)s6_addr[9],
                 (int)s6_addr[10], (int)s6_addr[11],
                 (int)s6_addr[12], (int)s6_addr[13],
                 (int)s6_addr[14], (int)s6_addr[15]);
}

ap_uint<MSGHDR_SEND_SIZE> sendmsg_A_to_B(ap_uint<32> size, ap_uint<64> id);
ap_uint<MSGHDR_SEND_SIZE> sendmsg_B_to_A(ap_uint<32> size, ap_uint<64> id);
ap_uint<MSGHDR_RECV_SIZE> recvmsg_A_to_B(ap_uint<64> id);
ap_uint<MSGHDR_RECV_SIZE> recvmsg_B_to_A();
void process();
void init();

void print_msghdr(struct c_msghdr_send_t * msghdr_send) {
    printf("sendmsg content dump:\n");

    char saddr[64];
    char daddr[64];

    // ipv6_to_str(saddr, msghdr_send->saddr);
    // ipv6_to_str(daddr, msghdr_send->daddr);

    printf("  - source address      : %s\n", saddr);
    printf("  - destination address : %s\n", daddr);
    printf("  - source port         : %u\n", (unsigned int) msghdr_send->sport);
    printf("  - dest port           : %u\n", (unsigned int) msghdr_send->dport);
    printf("  - buffer address      : %lx\n", msghdr_send->buff_addr);
    printf("  - buffer size         : %u\n",  msghdr_send->metadata);
    printf("  - rpc id              : %lu\n", msghdr_send->id);
    printf("  - completion cookie   : %lu\n", msghdr_send->cc);
}

ap_uint<MSGHDR_SEND_SIZE> sendmsg_A_to_B(ap_uint<32> size, ap_uint<64> id) {
    msghdr_send_t sendmsg_req;

    sendmsg_req.data(MSGHDR_SADDR)     = A_saddr;
    sendmsg_req.data(MSGHDR_DADDR)     = A_daddr;
    sendmsg_req.data(MSGHDR_SPORT)     = A_sport;
    sendmsg_req.data(MSGHDR_DPORT)     = A_dport;
    sendmsg_req.data(MSGHDR_BUFF_ADDR) = 0;
    sendmsg_req.data(MSGHDR_BUFF_SIZE) = size;
    sendmsg_req.data(MSGHDR_RETURN)    = rand() % 256;
    sendmsg_req.data(MSGHDR_SEND_ID)   = id;
    sendmsg_req.data(MSGHDR_SEND_CC)   = 0;

    *((ap_uint<MSGHDR_SEND_SIZE>*) (A_c2h_metadata + (sendmsg_req.data(MSGHDR_RETURN) * 64))) = 0;

    std::cerr << "SENDMSG START ID: " << id << " ret " << sendmsg_req.data(MSGHDR_RETURN) << std::endl;
    sendmsg_i.write(sendmsg_req);

    ap_uint<MSGHDR_SEND_SIZE> sendmsg_resp;
    while(true) {
	process();
	sendmsg_resp = *((ap_uint<MSGHDR_SEND_SIZE>*) (A_c2h_metadata + (sendmsg_req.data(MSGHDR_RETURN) * 64)));
	if (sendmsg_resp != 0) break;
    }

    std::cerr << "SENDMSG COMPLETE ID: " << id << " ret " << sendmsg_req.data(MSGHDR_RETURN) << std::endl;

    print_msghdr((struct c_msghdr_send_t *) &sendmsg_resp);

    return sendmsg_resp;
}

ap_uint<MSGHDR_SEND_SIZE> sendmsg_B_to_A(ap_uint<32> size, ap_uint<64> local_id, ap_uint<64> id) {
    msghdr_send_t sendmsg_req;

    sendmsg_req.data(MSGHDR_SADDR)     = B_saddr;
    sendmsg_req.data(MSGHDR_DADDR)     = B_daddr;
    sendmsg_req.data(MSGHDR_SPORT)     = B_sport;
    sendmsg_req.data(MSGHDR_DPORT)     = B_dport;
    sendmsg_req.data(MSGHDR_BUFF_ADDR) = 0;
    sendmsg_req.data(MSGHDR_BUFF_SIZE) = size;
    sendmsg_req.data(MSGHDR_RETURN)    = rand() % 256;
    sendmsg_req.data(MSGHDR_SEND_ID)   = local_id;
    sendmsg_req.data(MSGHDR_SEND_CC)   = id;

    *((ap_uint<MSGHDR_SEND_SIZE>*) (B_c2h_metadata + (sendmsg_req.data(MSGHDR_RETURN) * 64))) = 0;

    std::cerr << "SENDMSG START ID: " << id << " ret " << sendmsg_req.data(MSGHDR_RETURN) << std::endl;
    sendmsg_i.write(sendmsg_req);

    ap_uint<MSGHDR_SEND_SIZE> sendmsg_resp;
    while(true) {
	process();
	sendmsg_resp = *((ap_uint<MSGHDR_SEND_SIZE>*) (B_c2h_metadata + (sendmsg_req.data(MSGHDR_RETURN) * 64)));
	if (sendmsg_resp != 0) break;
    }

    std::cerr << "SENDMSG COMPLETE ID: " << id << " ret " << sendmsg_req.data(MSGHDR_RETURN) << std::endl;

    print_msghdr((struct c_msghdr_send_t *) &sendmsg_resp);

    return sendmsg_resp;
}

// TODO eventually randomly match on request or response or any
ap_uint<MSGHDR_RECV_SIZE> recvmsg_A_to_B(ap_uint<64> id) {
    msghdr_recv_t recvmsg_req;

    recvmsg_req.data(MSGHDR_SADDR)     = B_daddr;
    recvmsg_req.data(MSGHDR_DADDR)     = B_saddr;
    recvmsg_req.data(MSGHDR_SPORT)     = B_dport;
    recvmsg_req.data(MSGHDR_DPORT)     = B_sport;
    recvmsg_req.data(MSGHDR_BUFF_ADDR) = 0;
    recvmsg_req.data(MSGHDR_BUFF_SIZE) = 0;
    recvmsg_req.data(MSGHDR_RETURN)    = rand() % 256;
    recvmsg_req.data(MSGHDR_RECV_ID)   = id;
    recvmsg_req.data(MSGHDR_RECV_CC)   = 0;

    *((ap_uint<MSGHDR_RECV_SIZE>*) (B_c2h_metadata + (recvmsg_req.data(MSGHDR_RETURN) * 64))) = 0;

    std::cerr << "RECVMSG START ID: " << id << " ret " << recvmsg_req.data(MSGHDR_RETURN) << std::endl;
    recvmsg_i.write(recvmsg_req);

    ap_uint<MSGHDR_RECV_SIZE> recvmsg_resp;
    while(true) {
	process();
	recvmsg_resp = *((ap_uint<MSGHDR_RECV_SIZE>*) (B_c2h_metadata + (recvmsg_req.data(MSGHDR_RETURN) * 64)));
	if (recvmsg_resp != 0) break;
    }

    std::cerr << "RECV COMPLETE ID: " << recvmsg_resp(MSGHDR_RECV_ID) << " ret " << recvmsg_resp(MSGHDR_RETURN) << std::endl;

    return recvmsg_resp;
}


// TODO eventually randomly match on request or response or any
ap_uint<MSGHDR_RECV_SIZE> recvmsg_B_to_A(ap_uint<64> id) {
    msghdr_recv_t recvmsg_req;

    recvmsg_req.data(MSGHDR_SADDR)     = A_daddr;
    recvmsg_req.data(MSGHDR_DADDR)     = A_saddr;
    recvmsg_req.data(MSGHDR_SPORT)     = A_dport;
    recvmsg_req.data(MSGHDR_DPORT)     = A_sport;
    recvmsg_req.data(MSGHDR_BUFF_ADDR) = 0;
    recvmsg_req.data(MSGHDR_BUFF_SIZE) = 0;
    recvmsg_req.data(MSGHDR_RETURN)    = rand() % 256;
    recvmsg_req.data(MSGHDR_RECV_ID)   = id;
    recvmsg_req.data(MSGHDR_RECV_CC)   = 0;

    *((ap_uint<MSGHDR_RECV_SIZE>*) (A_c2h_metadata + (recvmsg_req.data(MSGHDR_RETURN) * 64))) = 0;

    std::cerr << "RECVMSG START ID: " << id << " ret " << recvmsg_req.data(MSGHDR_RETURN) << std::endl;
    recvmsg_i.write(recvmsg_req);

    ap_uint<MSGHDR_RECV_SIZE> recvmsg_resp;
    while(true) {
	process();
	recvmsg_resp = *((ap_uint<MSGHDR_RECV_SIZE>*) (A_c2h_metadata + (recvmsg_req.data(MSGHDR_RETURN) * 64)));
	if (recvmsg_resp != 0) break;
    }

    std::cerr << "RECV COMPLETE ID: " << recvmsg_resp(MSGHDR_RECV_ID) << " ret " << recvmsg_resp(MSGHDR_RETURN) << std::endl;

    return recvmsg_resp;
}

// short messages fail
void init() {
    A_h2c_msgbuff  = (char*) malloc(100000 * sizeof(char));
    A_c2h_msgbuff  = (char*) malloc(100000 * sizeof(char));
    A_c2h_metadata = (char*) malloc(16384 * sizeof(char));
    B_h2c_msgbuff  = (char*) malloc(100000 * sizeof(char));
    B_c2h_msgbuff  = (char*) malloc(100000 * sizeof(char));
    B_c2h_metadata = (char*) malloc(16384 * sizeof(char));

    A_c2h_msgbuff_map(PORT_TO_PHYS_ADDR) = (uint64_t) A_c2h_msgbuff;
    A_c2h_msgbuff_map(PORT_TO_PHYS_PORT) = A_sport;

    A_h2c_msgbuff_map(PORT_TO_PHYS_ADDR) = (uint64_t) A_h2c_msgbuff;
    A_h2c_msgbuff_map(PORT_TO_PHYS_PORT) = A_sport;

    A_c2h_metadata_map(PORT_TO_PHYS_ADDR) = (uint64_t) A_c2h_metadata;
    A_c2h_metadata_map(PORT_TO_PHYS_PORT) = A_sport;

    B_c2h_msgbuff_map(PORT_TO_PHYS_ADDR) = (uint64_t) B_c2h_msgbuff;
    B_c2h_msgbuff_map(PORT_TO_PHYS_PORT) = B_sport;

    B_h2c_msgbuff_map(PORT_TO_PHYS_ADDR) = (uint64_t) B_h2c_msgbuff;
    B_h2c_msgbuff_map(PORT_TO_PHYS_PORT) = B_sport;

    B_c2h_metadata_map(PORT_TO_PHYS_ADDR) = (uint64_t) B_c2h_metadata;
    B_c2h_metadata_map(PORT_TO_PHYS_PORT) = B_sport;

    h2c_port_to_msgbuff_i.write(A_h2c_msgbuff_map);
    c2h_port_to_msgbuff_i.write(A_c2h_msgbuff_map);
    c2h_port_to_metadata_i.write(A_c2h_metadata_map);

    h2c_port_to_msgbuff_i.write(B_h2c_msgbuff_map);
    c2h_port_to_msgbuff_i.write(B_c2h_msgbuff_map);
    c2h_port_to_metadata_i.write(B_c2h_metadata_map);
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

    memset(B_c2h_msgbuff, 0, 16384);
    memset(A_c2h_msgbuff, 0, 16384);
    memset(B_h2c_msgbuff, 0, 16384);
    memset(A_h2c_msgbuff, 0, 16384);

    for (int i = 0; i < (16384/4); ++i) memcpy(A_h2c_msgbuff + (i*4), &pattern, 4);
    ap_uint<32> size = 16385;
    // ap_uint<32> size = rand() % 4000;

    ap_uint<MSGHDR_SEND_SIZE> send = sendmsg_A_to_B(size, 0); // Request

    std::cerr << "SENDMSG A to B ID " << send(MSGHDR_SEND_ID) << std::endl;

    // TODO should be a REQUEST flag?
    ap_uint<MSGHDR_RECV_SIZE> recv = recvmsg_A_to_B(0); // Receipt

    while (memcmp(A_h2c_msgbuff, B_c2h_msgbuff, size) != 0) process();

    std::cerr << "RECVMSG LOCAL ID   " << recv(MSGHDR_RECV_ID) << std::endl;
    std::cerr << "RECVMSG NETWORK ID " << recv(MSGHDR_RECV_CC) << std::endl;

    send = sendmsg_B_to_A(size, recv(MSGHDR_RECV_ID), recv(MSGHDR_RECV_CC));

    std::cerr << "SENDMSG ID " << send(MSGHDR_SEND_ID) << std::endl;

    recv = recvmsg_B_to_A(0); // Receipt

    while (memcmp(B_h2c_msgbuff, A_c2h_msgbuff, size) != 0) process();

    std::cerr << "RECVMSG ID " << recv(MSGHDR_RECV_ID) << std::endl;
}

void print_log_entry(ap_uint<512> log_entry) {
    std::cerr << "Log Entry -- " << log_entry << std::endl;
    //std::cerr << "Log Entry -- ";

    //std::cerr << "DMA Wr "
    //if (log_entry(7,0) != 0) {
    //	std::cerr << "(" << log_entry(7,0) << ")";
    //} else {
    //	std::cerr << "(empty)";
    //}

    //std::cerr << "DMA Rd "
    //if (log_entry(15,8) != 0) {
    //	std::cerr << "(" << log_entry(15,8) << ")";
    //} else {
    //	std::cerr << "(empty)";
    //}

}

void dump_log() {
    log_control_i.write(LOG_DRAIN);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    log_entry_t log_entry;
    while (log_out_o.read_nb(log_entry)) {
	print_log_entry(log_entry.data);
    }
}	

int main(int argc, char **argv) {

    srand(time(NULL));

    homa(sendmsg_i,
	 recvmsg_i,
	 w_cmd_queue_o, w_data_queue_o, w_status_queue_i,
	 r_cmd_queue_o, r_data_queue_i, r_status_queue_i,
	 link_ingress_i, link_egress_o,
	 h2c_port_to_msgbuff_i, c2h_port_to_msgbuff_i,
	 c2h_port_to_metadata_i, log_control_i,
	 log_out_o
	);

    init();

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // random_rpc();

    for (int i = 0; i < 10; ++i) random_rpc();
    // dump_log();

    free(A_h2c_msgbuff);
    free(A_c2h_msgbuff);
    free(A_c2h_metadata);

    free(B_h2c_msgbuff);
    free(B_c2h_msgbuff);
    free(B_c2h_metadata);

    return 0;
}
