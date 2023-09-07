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

#ifdef CSIM
    homa(sendmsg_i, sendmsg_o, recvmsg_i, recvmsg_o, w_cmd_queue_o, w_data_queue_o, w_status_queue_i, r_cmd_queue_o, r_data_queue_i, r_status_queue_i, link_ingress_i, link_egress_o);

    ofstream trace_file;
    trace_file.open(string("../../../../traces/") + string(QUOTE(OFILE)));

    trace_file << SIG_MSGHDR_SEND_I << std::endl;
    trace_file << SIG_MSGHDR_RECV_I << std::endl;

    while (recvmsg_o.empty() || sendmsg_o.empty() || memcmp(maxi_in, maxi_out, MSG_SIZE) != 0) {
    	if (!link_egress_o.empty()) {
	    link_ingress_i.write(link_egress_o.read());
	    trace_file << SIG_EGR_O << std::endl;
	    trace_file << SIG_ING_I << std::endl;
	}

    	if (!r_cmd_queue_o.empty()) {
    	    am_cmd_t am_cmd = r_cmd_queue_o.read();
    	    ap_uint<512> read_data;
    	    am_status_t am_status;

	    trace_file << SIG_R_CMD_O << std::endl;

    	    memcpy((char*) &read_data, maxi_in + ((uint32_t) am_cmd(AM_CMD_SADDR)), am_cmd(AM_CMD_BTT) * sizeof(char));
    	    r_data_queue_i.write(read_data);
    	    r_status_queue_i.write(am_status);

	    trace_file << SIG_R_DATA_I << std::endl;
	    trace_file << SIG_R_STATUS_I << std::endl;
    	}

    	if (!w_cmd_queue_o.empty()) {

    	    am_cmd_t am_cmd = w_cmd_queue_o.read();
    	    ap_uint<512> write_data = w_data_queue_o.read();

	    trace_file << SIG_W_CMD_O << std::endl;
	    trace_file << SIG_W_DATA_O << std::endl;

    	    am_status_t am_status;

    	    memcpy(maxi_out + ((uint32_t) am_cmd(AM_CMD_SADDR)), (char*) &write_data, am_cmd(AM_CMD_BTT) * sizeof(char));
    	    w_status_queue_i.write(am_status);

	    trace_file << SIG_W_STATUS_I << std::endl;

    	}
    }

    recvmsg_o.read();
    sendmsg_o.read();

    trace_file << SIG_MSGHDR_SEND_O << std::endl;
    trace_file << SIG_MSGHDR_RECV_O << std::endl;

    trace_file << SIG_END << std::endl;

    trace_file.close();
#endif

#ifdef COSIM

    homa(sendmsg_i, sendmsg_o, recvmsg_i, recvmsg_o, w_cmd_queue_o, w_data_queue_o, w_status_queue_i, r_cmd_queue_o, r_data_queue_i, r_status_queue_i, link_ingress_i, link_egress_o, cmd_token_i);

    ifstream trace_file;
    trace_file.open(string("../../../../traces/") + string(QUOTE(OFILE)));

    uint32_t token;

    while (recvmsg_o.empty() || sendmsg_o.empty() || memcmp(maxi_in, maxi_out, MSG_SIZE) != 0) {
	if (trace_file >> token) {
	    cmd_token_i.write(token);
	}

    	if (!link_egress_o.empty()) {
	    std::cerr << "LINK CARRY\n";
	    link_ingress_i.write(link_egress_o.read());
	}

    	if (!r_cmd_queue_o.empty()) {
	    std::cerr << "READ REQUEST\n";
    	    am_cmd_t am_cmd = r_cmd_queue_o.read();
    	    ap_uint<512> read_data;
    	    am_status_t am_status;

    	    memcpy((char*) &read_data, maxi_in + ((uint32_t) am_cmd(AM_CMD_SADDR)), am_cmd(AM_CMD_BTT) * sizeof(char));
    	    r_data_queue_i.write(read_data);
    	    r_status_queue_i.write(am_status);
    	}

    	if (!w_cmd_queue_o.empty()) {
	    std::cerr << "WRITE REQUEST\n";
    	    am_cmd_t am_cmd = w_cmd_queue_o.read();
    	    ap_uint<512> write_data = w_data_queue_o.read();

    	    am_status_t am_status;

    	    memcpy(maxi_out + ((uint32_t) am_cmd(AM_CMD_SADDR)), (char*) &write_data, am_cmd(AM_CMD_BTT) * sizeof(char));
    	    w_status_queue_i.write(am_status);
    	}
    }

    recvmsg_o.read();
    sendmsg_o.read();

    trace_file.close();

#endif

    return 0;
}
