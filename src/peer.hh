#ifndef PEERMGMT_H
#define PEERMGMT_H

#include "homa.hh"
#include "net.hh"
#include "databuff.hh"

#define PEER_SUB_TABLE_SIZE 16384
#define PEER_SUB_TABLE_INDEX 14

#define MAX_PEERS 16384

#define NUM_PEER_UNACKED_IDS 5

#define PEER_HP_SIZE 4

struct peer_hashpack_t {
  ap_uint<128> s6_addr;

  bool operator==(const peer_hashpack_t & other) const {
    return (s6_addr == other.s6_addr); 
  }
};

struct homa_peer_t {
  peer_id_t peer_id;
  ap_uint<128> addr;
  //int unsched_cutoffs[HOMA_MAX_PRIORITIES];
  //ap_uint<16> cutoff_version;
  //unsigned long last_update_jiffies;
  //int outstanding_resends;
  //int most_recent_resend;
  //ap_uint<32> least_recent_ticks;
  //ap_uint<32> current_ticks;
  //int num_acks;
  //homa_ack_t acks[NUM_PEER_UNACKED_IDS];
};


void peer_map(hls::stream<new_rpc_t> & dma_ingress__peer_map,
	      hls::stream<new_rpc_t> & peer_map__rpc_state,
	      hls::stream<header_in_t> & chunk_ingress__peer_map);


#endif
