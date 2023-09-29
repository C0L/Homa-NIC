#include <sys/mman.h>
#include <unistd.h>
#include <fstream>
#include <fcntl.h>

#include "test.hh"

using namespace std;

int main(int argc, char **argv) {
    std::cerr << "****************************** SINGLE PACKET MESSAGE TEST ******************************" << endl;

    char * dest_file = NULL;
    char * src_file = NULL;
    int len;
    int c;

    opterr = 0;

    while ((c = getopt (argc, argv, "i:l:")) != -1)
	switch (c) {
	    case 'i':
		src_file = optarg;
		std::cerr << "PARSED SRC FILE " << optarg;
		break;
		//case 'o':
		//	dest_file = optarg;
		//	std::cerr << "PARSED DEST FILE " << optarg;
		//	break;
	    case 'l':
		len = atoi(optarg); 
		break;
	    default:
		abort();
	}
   
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
    recvmsg.data(MSGHDR_BUFF_ADDR)  = 0;
    recvmsg.data(MSGHDR_BUFF_SIZE)  = 0;
    recvmsg.data(MSGHDR_RECV_ID)    = 0;
    recvmsg.data(MSGHDR_RECV_CC)    = 0;
    recvmsg.data(MSGHDR_RECV_FLAGS) = HOMA_RECVMSG_REQUEST;

    sendmsg.data(MSGHDR_SADDR)     = saddr;
    sendmsg.data(MSGHDR_DADDR)     = daddr;
    sendmsg.data(MSGHDR_SPORT)     = sport;
    sendmsg.data(MSGHDR_DPORT)     = dport;
    sendmsg.data(MSGHDR_BUFF_ADDR) = 0;
    sendmsg.data(MSGHDR_BUFF_SIZE) = len;

    sendmsg.data(MSGHDR_SEND_ID)   = 0;
    sendmsg.data(MSGHDR_SEND_CC)   = 0;

    // TODO The file is not even that many bytes!?
    int maxi_in_fd = open(src_file, O_RDWR);
    char * maxi_in = (char*) mmap(NULL, 16384, PROT_WRITE, MAP_PRIVATE, maxi_in_fd, 0);

    // int maxi_out_fd = open(dest_file, O_RDWR);
    // char * maxi_out = (char*) mmap(NULL, 16384, PROT_WRITE, MAP_PRIVATE, maxi_out_fd, 0); 

    char * maxi_out = (char*) malloc(16384 * sizeof(char));

    recvmsg_i.write(recvmsg);
    sendmsg_i.write(sendmsg);

    // TODO munmap and free

    homa(sendmsg_i, sendmsg_o,
	 recvmsg_i, recvmsg_o,
	 w_cmd_queue_o, w_data_queue_o, w_status_queue_i,
	 r_cmd_queue_o, r_data_queue_i, r_status_queue_i,
	 link_ingress_i, link_egress_o,
	 h2c_port_to_phys_i, c2h_port_to_phys_i
	);

    // TODO wrong size comparison
    while (recvmsg_o.empty() || sendmsg_o.empty() || memcmp(maxi_in, maxi_out, len) != 0) {

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

    free(maxi_out);
    munmap(maxi_in, 16384);
    close(maxi_in_fd);

    return 0;
}
