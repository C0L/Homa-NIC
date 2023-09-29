#ifndef TEST_H
#define TEST_H

#include "homa.hh"

hls::stream<raw_stream_t>   link_ingress_i;
hls::stream<raw_stream_t>   link_egress_o;
hls::stream<msghdr_send_t>  sendmsg_i;
hls::stream<msghdr_send_t>  sendmsg_o;
hls::stream<msghdr_recv_t>  recvmsg_i;
hls::stream<msghdr_recv_t>  recvmsg_o;
hls::stream<am_cmd_t>       w_cmd_queue_o;
hls::stream<ap_uint<512>>   w_data_queue_o;
hls::stream<am_status_t>    w_status_queue_i;
hls::stream<am_cmd_t>       r_cmd_queue_o;
hls::stream<ap_uint<512>>   r_data_queue_i;
hls::stream<am_status_t>    r_status_queue_i;
hls::stream<port_to_phys_t> h2c_port_to_phys_i;
hls::stream<port_to_phys_t> c2h_port_to_phys_i;

#endif
