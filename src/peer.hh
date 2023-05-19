#ifndef PEERMGMT_H
#define PEERMGMT_H

#include "homa.hh"
#include "net.hh"
#include "xmitbuff.hh"

#define PEER_SUB_TABLE_SIZE 16384
#define PEER_SUB_TABLE_INDEX 14

#define MAX_PEERS 16384
#define MAX_PEERS_LOG2 14

#define NUM_PEER_UNACKED_IDS 5

typedef ap_uint<MAX_PEERS_LOG2> peer_id_t;

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
  int unsched_cutoffs[HOMA_MAX_PRIORITIES];
  ap_uint<16> cutoff_version;
  unsigned long last_update_jiffies;
  int outstanding_resends;
  int most_recent_resend;
  ap_uint<32> least_recent_ticks;
  ap_uint<32> current_ticks;
  int num_acks;
  homa_ack_t acks[NUM_PEER_UNACKED_IDS];
};

void update_peer_stack(hls::stream<peer_id_t> & peer_stack_next_primary,
		       hls::stream<peer_id_t> & peer_stack_next_secondary,
		       hls::stream<peer_id_t> & peer_stack_free);

void update_peer_table(hls::stream<peer_hashpack_t> & peer_table_request_primary,
		       hls::stream<peer_id_t> & peer_table_response_primary,
		       hls::stream<peer_hashpack_t> & peer_table_request_secondary,
		       hls::stream<peer_id_t> & peer_table_response_secondary,
		       hls::stream<homa_peer_t> & peer_table_insert_primary,
		       hls::stream<homa_peer_t> & peer_table_insert_secondary);

void update_peer_buffer(hls::stream<peer_id_t> & peer_buffer_request,
			hls::stream<homa_peer_t> & peer_buffer_response,
			hls::stream<homa_peer_t> & peer_buffer_insert_primary,
			hls::stream<homa_peer_t> & peer_buffer_insert_secondary);
#endif
