#ifndef INF_H
#define INF_H

#define MSGHDR_RECV_SIZE    512
typedef ap_axiu<MSGHDR_RECV_SIZE, 0, 0, 0> msghdr_recv_t;

#define MSGHDR_SEND_SIZE    512
typedef ap_axiu<MSGHDR_SEND_SIZE, 0, 0, 0> msghdr_send_t;

typedef ap_uint<512> port_to_phys_t;

void interface(
    ap_uint<512> * infmem,
    hls::stream<ap_uint<64>> & addr_in,
    hls::stream<msghdr_send_t> & sendmsg,
    hls::stream<msghdr_recv_t> & recvmsg,
    hls::stream<port_to_phys_t> & h2c_port_to_msgbuff,
    hls::stream<port_to_phys_t> & c2h_port_to_msgbuff,
    hls::stream<port_to_phys_t> & c2h_port_to_metadata
    );

#endif 
