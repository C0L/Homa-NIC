#ifndef USER_H
#define USER_H

#include "homa.hh"

void c2h_metadata(
    hls::stream<msghdr_recv_t> & msghdr_recv_i,
    hls::stream<ap_uint<MSGHDR_RECV_SIZE>> & msghdr_recv_o,
    hls::stream<header_t> & complete_msgs_i
    );

#endif
