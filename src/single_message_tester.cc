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

    recvmsg(MSGHDR_SADDR)      = saddr;
    recvmsg(MSGHDR_DADDR)      = daddr;
    recvmsg(MSGHDR_SPORT)      = sport;
    recvmsg(MSGHDR_DPORT)      = dport;
    recvmsg(MSGHDR_IOV)        = 0;
    recvmsg(MSGHDR_IOV_SIZE)   = 0;
    recvmsg(MSGHDR_RECV_ID)    = 0;
    recvmsg(MSGHDR_RECV_CC)    = 0;
    recvmsg(MSGHDR_RECV_FLAGS) = HOMA_RECVMSG_REQUEST;

    sendmsg(MSGHDR_SADDR)    = saddr;
    sendmsg(MSGHDR_DADDR)    = daddr;
    sendmsg(MSGHDR_SPORT)    = sport;
    sendmsg(MSGHDR_DPORT)    = dport;
    sendmsg(MSGHDR_IOV)      = 0;
    sendmsg(MSGHDR_IOV_SIZE) = MSG_SIZE;

    sendmsg(MSGHDR_SEND_ID)    = 0;
    sendmsg(MSGHDR_SEND_CC)    = 0;

    recvmsg_i.write(recvmsg);
    sendmsg_i.write(sendmsg);

    strcpy((char*) maxi_in, data.c_str());

#ifdef CSIM
    while (recvmsg_o.empty() || sendmsg_o.empty() || memcmp(maxi_in, maxi_out, MSG_SIZE) != 0) {
    	homa(0, 0, 0, 0, 0, 0, sendmsg_i, sendmsg_o, recvmsg_i, recvmsg_o, maxi_in, maxi_out, link_ingress_i, link_egress_o);
	if (!link_egress_o.empty()) link_ingress_i.write(link_egress_o.read());
    }

    recvmsg_o.read();
    sendmsg_o.read();
#endif

#ifdef COSIM	
    ifstream trace_file;
    trace_file.open(snprintf("../../../../traces/single_message_trace_%d_%d_%d", DMA_SIZE, RTT_BYTES, MSG_SIZE);

    uint32_t token;

    while (trace_file >> token) {
    	homa(token, token, token, token, token, token, sendmsg_i, sendmsg_o, recvmsg_i, recvmsg_o, maxi_in, maxi_out, link_ingress_i, link_egress_o);
    	if (!link_egress_o.empty()) link_ingress_i.write(link_egress_o.read());
    }

    recvmsg_o.read();
    sendmsg_o.read();
#endif

    return memcmp(maxi_in, maxi_out, MSG_SIZE);
}
