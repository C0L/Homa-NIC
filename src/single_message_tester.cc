#include "test.hh"

#include <unistd.h>

#include <fstream>

using namespace std;

int main() {
    std::cerr << "****************************** SINGLE PACKET MESSAGE TEST ******************************" << endl;
    std::cerr << "DMA SIZE  = " << DMA_SIZE << std::endl;
    std::cerr << "RTT BYTES = " << RTT_BYTES << std::endl;
    std::cerr << "MSG SIZE  = " << MSG_SIZE << std::endl;

    msghdr_send_t sendmsg;
    msghdr_recv_t recvmsg;

    ap_uint<128> saddr("DCBAFEDCBAFEDCBADCBAFEDCBAFEDCBA", 16);
    ap_uint<128> daddr("ABCDEFABCDEFABCDABCDEFABCDEFABCD", 16);

    uint16_t sport;
    uint16_t dport;

    dport = 0xBEEF;
    sport = 0xFEEB;

    recvmsg.data(MSGHDR_SADDR)      = saddr;
    recvmsg.data(MSGHDR_DADDR)      = daddr;
    recvmsg.data(MSGHDR_SPORT)      = sport;
    recvmsg.data(MSGHDR_DPORT)      = dport;
    recvmsg.data(MSGHDR_IOV)        = 0;
    recvmsg.data(MSGHDR_IOV_SIZE)   = 0;
    recvmsg.data(MSGHDR_RECV_ID)    = 0;
    recvmsg.data(MSGHDR_RECV_CC)    = 0;
    recvmsg.data(MSGHDR_RECV_FLAGS) = HOMA_RECVMSG_REQUEST;

    sendmsg.data(MSGHDR_SADDR)    = saddr;
    sendmsg.data(MSGHDR_DADDR)    = daddr;
    sendmsg.data(MSGHDR_SPORT)    = sport;
    sendmsg.data(MSGHDR_DPORT)    = dport;
    sendmsg.data(MSGHDR_IOV)      = 0;
    sendmsg.data(MSGHDR_IOV_SIZE) = MSG_SIZE;

    sendmsg.data(MSGHDR_SEND_ID)    = 0;
    sendmsg.data(MSGHDR_SEND_CC)    = 0;

    recvmsg_i.write(recvmsg);
    sendmsg_i.write(sendmsg);

    strcpy((char*) maxi_in, data.c_str());

    homa(sendmsg_i, sendmsg_o, recvmsg_i, recvmsg_o, w_cmd_queue_o, w_data_queue_o, w_status_queue_i, r_cmd_queue_o, r_data_queue_i, r_status_queue_i, link_ingress_i, link_egress_o);

    while (recvmsg_o.empty() || sendmsg_o.empty() || memcmp(maxi_in, maxi_out, MSG_SIZE) != 0) {
    	if (!link_egress_o.empty()) {
	    link_ingress_i.write(link_egress_o.read());
	}

    	if (!r_cmd_queue_o.empty()) {
    	    am_cmd_t am_cmd = r_cmd_queue_o.read();
    	    ap_uint<512> read_data;
    	    am_status_t am_status;

    	    memcpy((char*) &read_data, maxi_in + ((uint32_t) am_cmd(AM_CMD_SADDR)), am_cmd(AM_CMD_BTT) * sizeof(char));
    	    r_data_queue_i.write(read_data);
    	    r_status_queue_i.write(am_status);
    	}

    	if (!w_cmd_queue_o.empty()) {

    	    am_cmd_t am_cmd = w_cmd_queue_o.read();
    	    ap_uint<512> write_data = w_data_queue_o.read();

    	    am_status_t am_status;

    	    memcpy(maxi_out + ((uint32_t) am_cmd(AM_CMD_SADDR)), (char*) &write_data, am_cmd(AM_CMD_BTT) * sizeof(char));
    	    w_status_queue_i.write(am_status);
    	}
    }

    recvmsg_o.read();
    sendmsg_o.read();

    return 0;
}
