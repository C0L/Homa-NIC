#include "homa.hh"

#include <unistd.h>

#include <fstream>

using namespace std;

const std::string data = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Velit aliquet sagittis id consectetur purus. Ut porttitor leo a diam sollicitudin. Tristique senectus et netus et malesuada fames ac turpis. Et ligula ullamcorper malesuada proin libero nunc. Sit amet tellus cras adipiscing enim eu turpis. Vel eros donec ac odio tempor orci dapibus. Quam viverra orci sagittis eu volutpat odio. Enim neque volutpat ac tincidunt vitae semper. Nunc sed velit dignissim sodales. Magna eget est lorem ipsum dolor sit amet consectetur adipiscing. Ut etiam sit amet nisl purus. Tristique senectus et netus et malesuada fames ac turpis. Viverra mauris in aliquam sem fringilla ut morbi tincidunt. Suspendisse interdum consectetur libero id faucibus nisl tincidunt eget. Ipsum a arcu cursus vitae.        At quis risus sed vulputate odio ut enim blandit volutpat. A erat nam at lectus urna duis convallis convallis. Gravida quis blandit turpis cursus in hac habitasse. Commodo elit at imperdiet dui accumsan sit amet nulla. Elit pellentesque habitant morbi tristique senectus et netus et. Vulputate eu scelerisque felis imperdiet proin fermentum. Mauris in aliquam sem fringilla ut. Morbi tincidunt ornare massa eget egestas purus. At tempor commodo ullamcorper a lacus vestibulum sed arcu. Sit amet nulla facilisi morbi tempus iaculis. Morbi tristique senectus et netus et malesuada fames ac turpis. Sagittis aliquam malesuada bibendum arcu. Vivamus arcu felis bibendum ut tristique et egestas quis. Cursus sit amet dictum sit amet justo donec. Porttitor rhoncus dolor purus non enim praesent elementum facilisis. Sagittis aliquam malesuada bibendum arcu vitae elementum.       Mattis vulputate enim nulla aliquet porttitor lacus luctus accumsan. Dictum varius duis at consectetur lorem donec massa sapien faucibus. Nisi quis eleifend quam adipiscing vitae. Suspendisse faucibus interdum posuere lorem ipsum dolor sit amet consectetur. Tincidunt arcu non sodales neque sodales ut etiam. Id volutpat lacus laoreet non curabitur gravida arcu ac. Nulla facilisi cras fermentum odio eu feugiat. Fames ac turpis egestas sed tempus urna. Tristique nulla aliquet enim tortor at auctor urna. Bibendum neque egestas congue quisque egestas diam in arcu. Pharetra diam sit amet nisl. Etiam non quam lacus suspendisse faucibus. Diam sit amet nisl suscipit adipiscing bibendum est. At ultrices mi tempus imperdiet nulla malesuada pellentesque. Auctor neque vitae tempus quam pellentesque. Facilisis leo vel fringilla est ullamcorper eget nulla facilisi etiam. Morbi tempus iaculis urna id volutpat lacus laoreet non. Id semper risus in hendrerit gravida. Tincidunt dui ut ornare lectus sit. Ac odio tempor orci     Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Velit aliquet sagittis id consectetur purus. Ut porttitor leo a diam sollicitudin. Tristique senectus et netus et malesuada fames ac turpis. Et ligula ullamcorper malesuada proin libero nunc. Sit amet tellus cras adipiscing enim eu turpis. Vel eros donec ac odio tempor orci dapibus. Quam viverra orci sagittis eu volutpat odio. Enim neque volutpat ac tincidunt vitae semper. Nunc sed velit dignissim sodales. Magna eget est lorem ipsum dolor sit amet consectetur adipiscing. Ut etiam sit amet nisl purus. Tristique senectus et netus et malesuada fames ac turpis. Viverra mauris in aliquam sem fringilla ut morbi tincidunt. Suspendisse interdum consectetur libero id faucibus nisl tincidunt eget. Ipsum a arcu cursus vitae.        At quis risus sed vulputate odio ut enim blandit volutpat. A erat nam at lectus urna duis convallis convallis. Gravida quis blandit turpis cursus in hac habitasse. Commodo elit at imperdiet dui accumsan sit amet nulla. Elit pellentesque habitant morbi tristique senectus et netus et. Vulputate eu scelerisque felis imperdiet proin fermentum. Mauris in aliquam sem fringilla ut. Morbi tincidunt ornare massa eget egestas purus. At tempor commodo ullamcorper a lacus vestibulum sed arcu. Sit amet nulla facilisi morbi tempus iaculis. Morbi tristique senectus et netus et malesuada fames ac turpis. Sagittis aliquam malesuada bibendum arcu. Vivamus arcu felis bibendum ut tristique et egestas quis. Cursus sit amet dictum sit amet justo donec. Porttitor rhoncus dolor purus non enim praesent elementum facilisis. Sagittis aliquam malesuada bibendum arcu vitae elementum.       Mattis vulputate enim nulla aliquet porttitor lacus luctus accumsan. Dictum varius duis at consectetur lorem donec massa sapien faucibus. Nisi quis eleifend quam adipiscing vitae. Suspendisse faucibus interdum posuere lorem ipsum dolor sit amet consectetur. Tincidunt arcu non sodales neque sodales ut etiam. Id volutpat lacus laoreet non curabitur gravida arcu ac. Nulla facilisi cras fermentum odio eu feugiat. Fames ac turpis egestas sed tempus urna. Tristique nulla aliquet enim tortor at auctor urna. Bibendum neque egestas congue quisque egestas diam in arcu. Pharetra diam sit amet nisl. Etiam non quam lacus suspendisse faucibus. Diam sit amet nisl suscipit adipiscing bibendum est. At ultrices mi tempus imperdiet nulla malesuada pellentesque. Auctor neque vitae tempus quam pellentesque. Facilisis leo vel fringilla est ullamcorper eget nulla facilisi etiam. Morbi tempus iaculis urna id volutpat lacus laoreet non. Id semper risus in hendrerit gravida. Tincidunt dui ut ornare lectus sit. Ac odio tempor orci";

int main() {
    std::cerr << "****************************** START TEST BENCH ******************************" << endl;

    // 256 is needed for the verification adapter!?!
    hls::stream<raw_stream_t, 512> link_ingress_i;
    hls::stream<raw_stream_t, 512> link_egress_o;
    hls::stream<msghdr_send_t, 512> sendmsg_i;
    hls::stream<msghdr_send_t, 512> sendmsg_o;
    hls::stream<msghdr_recv_t, 512> recvmsg_i;
    hls::stream<msghdr_recv_t, 512> recvmsg_o;

    msghdr_send_t sendmsg;
    msghdr_recv_t recvmsg;

    ap_uint<128> saddr("DCBAFEDCBAFEDCBADCBAFEDCBAFEDCBA", 16);
    ap_uint<128> daddr("ABCDEFABCDEFABCDABCDEFABCDEFABCD", 16);

    uint16_t sport;
    uint16_t dport;

    dport = 0xBEEF;
    sport = 0xFEEB;

    // Offset in DMA space, receiver address, sender address, receiver port, sender port, RPC ID (0 for match-all)

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
    sendmsg(MSGHDR_IOV_SIZE) = 2772;

    sendmsg(MSGHDR_SEND_ID)    = 0;
    sendmsg(MSGHDR_SEND_CC)    = 0;

    ap_uint<512> maxi_in[128];
    ap_uint<512> maxi_out[128];

    recvmsg_i.write(recvmsg);
    sendmsg_i.write(sendmsg);

    strcpy((char*) maxi_in, data.c_str());

    // TODO clear out the old file

#ifdef CSIM
    // Generate the test vector
    // while (recvmsg_o.empty() || sendmsg_o.empty()) {
    // while (recvmsg_o.empty() || sendmsg_o.empty() || ((char *) maxi_out)[2772] == 0) {
    while (recvmsg_o.empty() || sendmsg_o.empty() || memcmp(maxi_in, maxi_out, 2772) != 0) {
	// TODO might be good to stall in between so that all the possible actions to take are outputed in trace
	uint32_t tokens[6];

    	homa(0, 0, 0, 0, 0, 0, sendmsg_i, sendmsg_o, recvmsg_i, recvmsg_o, maxi_in, maxi_out, link_ingress_i, link_egress_o);
	if (!link_egress_o.empty()) link_ingress_i.write(link_egress_o.read());
    }


    char * t0 = (char *) &(maxi_in);
    char * t1 = (char *) &(maxi_out);
    
       // std::cerr << "DATA: " << test << std::endl;
       std::cerr << "DATA: ";
       for (int i = 0; i < 2772; ++i) {
	   std::cerr << t0[i]; 
       }

       std::cerr << std::endl;
       std::cerr << std::endl;
       std::cerr << std::endl;
       std::cerr << std::endl;
       std::cerr << std::endl;
       std::cerr << std::endl;
       // std::cerr << "DATA: " << test << std::endl;
       std::cerr << "DATA: ";
       for (int i = 0; i < 2772; ++i) {
	   std::cerr << t1[i]; 
       }

       std::cerr << std::endl;


    recvmsg_o.read();
    sendmsg_o.read();
#endif

#ifdef COSIM	
    ifstream trace_file;
    trace_file.open("../../../../traces/unscheduled_trace");

    uint32_t token;

    while (trace_file >> token) {
    	std::cerr << token << std::endl;
	//uint32_t tokens[6];
	//tokens[0] = token;
	//tokens[1] = token;
	//tokens[2] = token;
	//tokens[3] = token;
	//tokens[4] = token;
	//tokens[5] = token;
	
    	homa(token, token, token, token, token, token, sendmsg_i, sendmsg_o, recvmsg_i, recvmsg_o, maxi_in, maxi_out, link_ingress_i, link_egress_o);
    	if (!link_egress_o.empty()) link_ingress_i.write(link_egress_o.read());
    }

    recvmsg_o.read();
    sendmsg_o.read();
#endif

    // std::cerr << string((char*) maxi_out) << std::endl;

    // return 0;
    return memcmp(maxi_in, maxi_out, 2772);
}
