#include <sys/mman.h>
#include <unistd.h>
#include <fstream>
#include <fcntl.h>

#include <chrono>
#include <thread>

#include "test.hh"

using namespace std;

ap_uint<128> saddr("DCBAFEDCBAFEDCBADCBAFEDCBAFEDCBA", 16);
ap_uint<128> daddr("ABCDEFABCDEFABCDABCDEFABCDEFABCD", 16);

uint16_t sport = 1;
uint16_t dport = 1;

ap_uint<64> c2h_phys = 0xDEADBEEF;
ap_uint<64> h2c_phys = 0xBEEFDEAD;

port_to_phys_t c2h_address_map;
port_to_phys_t h2c_address_map;

char * h2c_buffer;
char * c2h_buffer;


void process();
void sendmsg(ap_uint<32> size, ap_uint<64> id);
msghdr_recv_t recvmsg();
void init();

void sendmsg(ap_uint<32> size, ap_uint<64> id) {
    msghdr_send_t sendmsg;

    sendmsg.data(MSGHDR_SADDR)     = saddr;
    sendmsg.data(MSGHDR_DADDR)     = daddr;
    sendmsg.data(MSGHDR_SPORT)     = 1;
    sendmsg.data(MSGHDR_DPORT)     = 1;
    sendmsg.data(MSGHDR_BUFF_ADDR) = 0;
    sendmsg.data(MSGHDR_BUFF_SIZE) = size;

    sendmsg.data(MSGHDR_SEND_ID)   = 0;
    sendmsg.data(MSGHDR_SEND_CC)   = 0;

    sendmsg_i.write(sendmsg);
}

msghdr_recv_t recvmsg() {
    msghdr_recv_t recvmsg;

    recvmsg.data(MSGHDR_SADDR)      = saddr;
    recvmsg.data(MSGHDR_DADDR)      = daddr;
    recvmsg.data(MSGHDR_SPORT)      = 1;
    recvmsg.data(MSGHDR_DPORT)      = 1;
    recvmsg.data(MSGHDR_BUFF_ADDR)  = 0;
    recvmsg.data(MSGHDR_BUFF_SIZE)  = 0;
    recvmsg.data(MSGHDR_RECV_ID)    = 0;
    recvmsg.data(MSGHDR_RECV_CC)    = 0;
    recvmsg.data(MSGHDR_RECV_FLAGS) = HOMA_RECVMSG_REQUEST;

    recvmsg_i.write(recvmsg);

    while (!recvmsg_o.read_nb(recvmsg)) process();

    return recvmsg;
}

void init() {
    h2c_buffer = (char*) malloc(16384 * sizeof(char));
    c2h_buffer = (char*) malloc(16384 * sizeof(char));

    c2h_address_map(PORT_TO_PHYS_ADDR) = c2h_phys;
    c2h_address_map(PORT_TO_PHYS_PORT) = sport;

    h2c_address_map(PORT_TO_PHYS_ADDR) = h2c_phys;
    h2c_address_map(PORT_TO_PHYS_PORT) = sport;

    h2c_port_to_phys_i.write(h2c_address_map);
    c2h_port_to_phys_i.write(c2h_address_map);
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
	
	memcpy((char*) &read_data, h2c_buffer + ((uint32_t) (am_cmd.data(AM_CMD_SADDR) - h2c_address_map(PORT_TO_PHYS_ADDR))), am_cmd.data(AM_CMD_BTT) * sizeof(char));
	r_data_queue_i.write(read_data);
	r_status_queue_i.write(am_status);
    }
    
    if (!w_cmd_queue_o.empty()) {
	
	am_cmd_t am_cmd = w_cmd_queue_o.read();
	ap_axiu<512,0,0,0> write_data = w_data_queue_o.read();
	
	am_status_t am_status;
	
	memcpy(c2h_buffer + ((uint32_t) (am_cmd.data(AM_CMD_SADDR) - c2h_address_map(PORT_TO_PHYS_ADDR))), (char*) &write_data.data, am_cmd.data(AM_CMD_BTT) * sizeof(char));
	w_status_queue_i.write(am_status);
    }
}

int main(int argc, char **argv) {
    std::cerr << "****************************** SINGLE PACKET MESSAGE TEST ******************************" << endl;

    homa(sendmsg_i, sendmsg_o,
	 recvmsg_i, recvmsg_o,
	 w_cmd_queue_o, w_data_queue_o, w_status_queue_i,
	 r_cmd_queue_o, r_data_queue_i, r_status_queue_i,
	 link_ingress_i, link_egress_o,
	 h2c_port_to_phys_i, c2h_port_to_phys_i,
	 log_out
	);

    init();

    // TODO need to check data contents are correct??

    // Request message
    sendmsg(10, 0);

    recvmsg();

    //while (!log_out.empty()) {
    //	log_entry_t log_entry = log_out.read();
    //	std::cerr << "LOG OUT READ: " << log_entry.data(63, 0) << std::endl;
    //	std::cerr << "LOG OUT WRITE: " << log_entry.data(127, 64) << std::endl;
    //}

    //recvmsg_o.read();
    //sendmsg_o.read();

    free(h2c_buffer);
    free(c2h_buffer);

    return 0;
}
