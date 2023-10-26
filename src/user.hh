#ifndef USER_H
#define USER_H

#include "homa.hh"

void c2h_metadata(
    hls::stream<msghdr_send_t> & sendmsg_i,
    hls::stream<dma_w_req_t> & sendmsg_o,
    hls::stream<homa_rpc_t> & new_rpc_o,
    hls::stream<msghdr_recv_t> & msghdr_recv_i,
    hls::stream<header_t> & complete_msgs_i,
    hls::stream<dma_w_req_t> & msghdr_recv_o,
    hls::stream<port_to_phys_t> & c2h_port_to_metadata_i,
    hls::stream<local_id_t> & new_client_i,
    hls::stream<dbuff_id_t> & new_dbuff_i
    );

#endif
