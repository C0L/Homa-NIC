#ifndef RPCMGMT_H
#define RPCMGMT_H

#include "homa.hh"
#include "srptmgmt.hh"
#include "databuff.hh"
#include "stack.hh"

void rpc_state(
    hls::stream<msghdr_send_t> & sendmsg_i,
    hls::stream<dma_w_req_t> & sendmsg_dma_o,
    hls::stream<srpt_queue_entry_t> & data_queue_o,
    hls::stream<srpt_queue_entry_t> & fetch_queue_o,
    hls::stream<srpt_grant_new_t> & grant_queue_o,
    hls::stream<ap_uint<MSGHDR_RECV_SIZE>> & recvmsg_i,
    hls::stream<dma_w_req_t> & recvmsg_dma_o,
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
    hls::stream<port_to_phys_t> & c2h_port_to_metadata_i,
    hls::stream<srpt_queue_entry_t> & dma_r_req_i,
    hls::stream<dma_r_req_t> & dma_r_req_o,
    hls::stream<dbuff_id_t> & free_dbuff_i,
    hls::stream<ap_uint<8>> & h2c_pkt_log_o,
    hls::stream<ap_uint<8>> & c2h_pkt_log_o
    );

#endif
