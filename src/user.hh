#ifndef USER_H
#define USER_H

#include "homa.hh"

void homa_recvmsg(hls::stream<msghdr_recv_t> & msghdr_recv_i,
      hls::stream<msghdr_recv_t> & msghdr_recv_o,
      hls::stream<header_t> & header_in_i);

void homa_sendmsg(hls::stream<msghdr_send_t> & msghdr_send_i,
      hls::stream<msghdr_send_t> & msghdr_send_o,
      hls::stream<onboard_send_t> & onboard_send_o);

#endif
