#ifndef RPCMGMT_H
#define RPCMGMT_H

#include "homa.hh"
#include "peer.hh"
#include "net.hh"
#include "xmitbuff.hh"
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
  xmit_id_t xmit_id;
  int next_xmit_offset;
  int unscheduled;
  int granted;
  ap_uint<8> sched_priority;
  ap_uint<64> init_cycles;
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
  peer_id_t peer;
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

void update_rpc_stack(hls::stream<rpc_id_t> & rpc_stack_next_primary,
		      hls::stream<rpc_id_t> & rpc_stack_next_secondary,
		      hls::stream<rpc_id_t> & rpc_stack_free);

void update_rpc_table(hls::stream<rpc_hashpack_t> & rpc_table_request_primary,
		      hls::stream<rpc_id_t> & rpc_table_response_primary,
		      hls::stream<rpc_hashpack_t> & rpc_table_request_secondary,
		      hls::stream<rpc_id_t> & rpc_table_response_secondary,
     		      hls::stream<homa_rpc_t> & rpc_table_insert);

void update_rpc_buffer(hls::stream<rpc_id_t> & rpc_buffer_request_primary,
		       hls::stream<homa_rpc_t> & rpc_buffer_response_primary,
		       hls::stream<pending_pkt_t> & pending_pkt_in,
		       hls::stream<pending_pkt_t> & pending_pkt_out,
		       hls::stream<rpc_id_t> & rpc_buffer_request_ternary,
		       hls::stream<homa_rpc_t> & rpc_buffer_response_ternary,
		       hls::stream<homa_rpc_t> & rpc_buffer_insert_primary,
		       hls::stream<homa_rpc_t> & rpc_buffer_insert_secondary);


#endif
