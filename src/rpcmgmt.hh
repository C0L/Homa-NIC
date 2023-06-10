#ifndef RPCMGMT_H
#define RPCMGMT_H

#include "homa.hh"
#include "srptmgmt.hh"
#include "peer.hh"
#include "net.hh"
#include "databuff.hh"
#include "stack.hh"


#define MAX_RPCS 16384

#define RPC_SUB_TABLE_SIZE 16384
#define RPC_SUB_TABLE_INDEX 14

#define MAX_OPS 64

/**
 * struct homa_message_out - Describes a message (either request or response)
 * for which this machine is the sender.
 */
struct homa_message_out_t {
  int length;
  dbuff_id_t dbuff_id;
  int unscheduled;
  int granted;
  //ap_uint<8> sched_priority;
  //ap_uint<64> init_cycles;
};

/**
 * struct homa_message_in - Holds the state of a message received by
 * this machine; used for both requests and responses.
 */
struct homa_message_in_t {
  int total_length;
  int bytes_remaining;
  int incoming;
  int priority;
  bool scheduled;
  ap_uint<64> birth;
  int copied_out;
};

/**
 * struct homa_rpc - One of these structures exists for each active
 * RPC. The same structure is used to manage both outgoing RPCs on
 * clients and incoming RPCs on servers.
 */
struct homa_rpc_t {
  uint32_t buffout;
  uint32_t buffin;
  enum {
    RPC_OUTGOING            = 5,
    RPC_INCOMING            = 6,
    RPC_IN_SERVICE          = 8,
    RPC_DEAD                = 9
  } state;

#define RPC_PKTS_READY        1
#define RPC_COPYING_FROM_USER 2
#define RPC_COPYING_TO_USER   4
#define RPC_HANDING_OFF       8
#define RPC_CANT_REAP (RPC_COPYING_FROM_USER | RPC_COPYING_TO_USER | RPC_HANDING_OFF)

  int flags;
  int grants_in_progress;
  ap_uint<128> saddr;
  ap_uint<128> daddr;
  uint16_t dport;
  uint16_t sport;
  peer_id_t peer_id;
  rpc_id_t rpc_id; 
  ap_uint<64> completion_cookie;
  homa_message_in_t msgin;
  homa_message_out_t msgout;
  int silent_ticks;
  ap_uint<32> resend_timer_ticks;
  ap_uint<32> done_timer_ticks;
};

#define RPC_HP_SIZE 7
struct rpc_hashpack_t {
  ap_uint<128> s6_addr;
  uint64_t id;
  uint16_t port;
  uint16_t empty;

  bool operator==(const rpc_hashpack_t & other) const {
      return (s6_addr == other.s6_addr && id == other.id && port == other.port);
  }
};

void rpc_state(hls::stream<new_rpc_t> & peer_map__rpc_state,
	       hls::stream<new_rpc_t> & rpc_state__srpt_data,
	       hls::stream<header_out_t> & egress_sel__rpc_state, 
	       hls::stream<header_out_t> & rpc_state__chunk_dispatch,
	       hls::stream<srpt_grant_t> & rpc_state__srpt_grant,
	       hls::stream<header_in_t> & rpc_state__dbuff_ingress);

#endif
