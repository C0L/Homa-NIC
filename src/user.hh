#ifndef USER_H
#define USER_H

#include "homa.hh"

void h2c_address_map(
    hls::stream<port_to_phys_t> & h2c_port_to_phys_i,
    hls::stream<homa_rpc_t> & sendmsg_i,
    hls::stream<homa_rpc_t> & sendmsg_o,
    hls::stream<srpt_queue_entry_t> & dma_r_req_i,
    hls::stream<dma_r_req_t> & dma_r_req_o);

void c2h_address_map(
    hls::stream<port_to_phys_t> & c2h_port_to_phys_i,
    hls::stream<header_t> & c2h_header_i,
    hls::stream<header_t> & c2h_header_o,
    hls::stream<dma_w_req_t> & dma_w_req_i,
    hls::stream<dma_w_req_t> & dma_w_req_o);

void homa_recvmsg(hls::stream<msghdr_recv_t> & msghdr_recv_i,
		  hls::stream<msghdr_recv_t> & msghdr_recv_o,
		  hls::stream<header_t>      & header_in_i);

void homa_sendmsg(hls::stream<msghdr_send_t> & msghdr_send_i,
		  hls::stream<msghdr_send_t> & msghdr_send_o,
		  hls::stream<homa_rpc_t>    & onboard_send_o);
#endif
