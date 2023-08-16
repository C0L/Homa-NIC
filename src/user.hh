#ifndef USER_H
#define USER_H

#include "homa.hh"

#ifdef STEPPED
void homa_recvmsg(bool recv_in_en,
		  bool recv_out_en,
		  hls::stream<msghdr_recv_t> & msghdr_recv_i,
		  hls::stream<msghdr_recv_t> & msghdr_recv_o,
		  hls::stream<header_t> & header_in_i);
#else
void homa_recvmsg(hls::stream<msghdr_recv_t> & msghdr_recv_i,
		  hls::stream<msghdr_recv_t> & msghdr_recv_o,
		  hls::stream<header_t> & header_in_i);
#endif

#ifdef STEPPED
void homa_sendmsg(bool send_in_en,
		  bool send_out_en,
		  hls::stream<msghdr_send_t> & msghdr_send_i,
		  hls::stream<msghdr_send_t> & msghdr_send_o,
		  hls::stream<onboard_send_t> & onboard_send_o);
#else
void homa_sendmsg(hls::stream<msghdr_send_t> & msghdr_send_i,
		  hls::stream<msghdr_send_t> & msghdr_send_o,
		  hls::stream<onboard_send_t> & onboard_send_o);
#endif



#endif
