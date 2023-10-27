#ifndef RPCMGMT_H
#define RPCMGMT_H

#include "homa.hh"
#include "srptmgmt.hh"
#include "databuff.hh"
#include "stack.hh"


void c2h_header_hashmap(
    hls::stream<header_t> & c2h_header_i,
    hls::stream<header_t> & c2h_header_o,
    hls::stream<entry_t<rpc_hashpack_t, local_id_t>> & new_rpcmap_entry,
    hls::stream<entry_t<peer_hashpack_t, peer_id_t>> & new_peermap_entry
    );



void c2h_header_cam(
    hls::stream<header_t> & c2h_header_i,
    hls::stream<header_t> & c2h_header_o,
    hls::stream<entry_t<rpc_hashpack_t, local_id_t>> & new_rpcmap_entry,
    hls::stream<entry_t<peer_hashpack_t, peer_id_t>> & new_peermap_entry,
    hls::stream<local_id_t> & new_server,
    hls::stream<peer_id_t> & new_peer
    );

void rpc_state(
    hls::stream<homa_rpc_t> & new_rpc_i,
    hls::stream<srpt_queue_entry_t> & data_queue_o,
    hls::stream<srpt_queue_entry_t> & fetch_queue_o,
    hls::stream<srpt_grant_new_t> & grant_queue_o,
    hls::stream<header_t> & h2c_header_i,
    hls::stream<header_t> & h2c_header_o,
    hls::stream<header_t> & c2h_header_i,
    hls::stream<header_t> & c2h_header_o,
    hls::stream<srpt_queue_entry_t> & data_srpt_o,
    hls::stream<srpt_queue_entry_t> & dbuff_notif_i,
    hls::stream<srpt_queue_entry_t> & dbuff_notif_o,
    hls::stream<port_to_phys_t> & c2h_port_to_msgbuff_i,
    hls::stream<dma_w_req_t> & dma_w_req_i,
    hls::stream<dma_w_req_t> & dma_w_req_o,
    hls::stream<port_to_phys_t> & h2c_port_to_msgbuff_i,
    hls::stream<srpt_queue_entry_t> & dma_r_req_i,
    hls::stream<dma_r_req_t> & dma_r_req_o,
    hls::stream<dbuff_id_t> & free_dbuff_i,
    hls::stream<dbuff_id_t> & new_dbuff_o,
    hls::stream<local_id_t> & new_client_o,
    hls::stream<local_id_t> & new_server_o,
    hls::stream<local_id_t> & new_peer_o,
    hls::stream<ap_uint<8>> & h2c_pkt_log_o,
    hls::stream<ap_uint<8>> & c2h_pkt_log_o
    );

#endif
