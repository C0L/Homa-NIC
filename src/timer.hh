#ifndef TIMER_H
#define TIMER_H

#include "rpcmgmt.hh"

// 1ms == 1000000 == 200000 cycles
#define REXMIT_CUTOFF 200000

// HOMA_MAX_MESSAGE_LENGTH / (ETHERNET_MAX_PAYLOAD(1500) - IPV6_HEADER_LENGTH(40) - HOMA_MAX_DATA_PKT_HEADER(60))

// Index into a packetmap bitmap
typedef ap_uint<10> packetmap_idx_t;

struct packetmap_t {
  packetmap_idx_t head;
  packetmap_idx_t length;
  ap_uint<64> map;
};

struct touch_t {
  rpc_id_t rpc_id;

  bool init;

  // Bit to set in packet map
  packetmap_idx_t offset;

  // Total length of the message
  packetmap_idx_t length;
};


struct rexmit_t {
  rpc_id_t rpc_id;

  // Bit to set in packet map
  packetmap_idx_t offset;
};

void rexmit_buffer(hls::stream<touch_t> & touch_in, 
		   hls::stream<rpc_id_t> & complete_out,
		   hls::stream<rexmit_t> & rexmit_out);

#endif
