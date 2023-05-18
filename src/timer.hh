#ifndef TIMER_H
#define TIMER_H

#include "rpcmgmt.hh"

// 1ms == 1000000 == 200000 cycles
#define REXMIT_CUTOFF 200000

// The number of packets worth of data to fit an entire maximum homa message
// HOMA_MAX_MESSAGE_LENGTH / (ETHERNET_MAX_PAYLOAD(1500) - IPV6_HEADER_LENGTH(40) - HOMA_MAX_DATA_PKT_HEADER(60))

#define PACKETMAP_WIDTH 715

typedef packetmap_t ap_uint<PACKETMAP_WIDTH>;

// Index into a packetmap bitmap
typedef packetmap_idx_t ap_uint<10>;

struct rexmit_t {
  rpc_id_t rpc_id;
  packetmap_idx_t offset;
};


void update_timer(hls::stream<ap_uint<1>> & timer_request_0,
		  hls::stream<ap_uint<64>> & timer_response_0);

void rexmit_buffer(hls::stream<rpc_id_t> & touch,
		   hls::stream<rpc_id_t> & rexmit);

#endif
