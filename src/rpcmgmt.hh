#ifndef RPCMGMT_H
#define RPCMGMT_H

#include "homa.hh"
#include "net.hh"
#include "xmitbuff.hh"

#define RPC_SUB_TABLE_SIZE 16384
#define RPC_SUB_TABLE_INDEX 14

// TODO this is the max number of slots but the cuckoo will not behave correctly if >95% saturated
// The number of RPCs supported on the device
#define MAX_RPCS 16384
//#define MAX_RPCS 65536

// Number of bits needed to index MAX_RPCS+1 (log2 MAX_RPCS + 1)
#define MAX_RPCS_LOG2 14
//#define MAX_RPCS_LOG2 16

// To index all the RPCs, we need LOG2 of the max number of RPCs
typedef ap_uint<MAX_RPCS_LOG2> rpc_id_t;

#define MAX_OPS 64

#define SEED0 0x7BF6BF21
#define SEED1 0x9FA91FE9
#define SEED2 0xD0C8FBDF
#define SEED3 0xE6D0851C

#define PEER_SUB_TABLE_SIZE 16384
#define PEER_SUB_TABLE_INDEX 14

// TODO temporary value 
#define MAX_PEERS 16384

#define MAX_PEERS_LOG2 14

typedef ap_uint<MAX_PEERS_LOG2> peer_id_t;

#define NUM_PEER_UNACKED_IDS 5

struct homa_peer_t {
  peer_id_t peer_id;
  ap_uint<128> addr;
  int unsched_cutoffs[HOMA_MAX_PRIORITIES];
  ap_uint<16> cutoff_version;
  unsigned long last_update_jiffies;

  // TODO not sure how to reintegrate this
  ///**
  // * grantable_rpcs: Contains all homa_rpcs (both requests and
  // * responses) involving this peer whose msgins require (or required
  // * them in the past) and have not been fully received. The list is
  // * sorted in priority order (head has fewest bytes_remaining).
  // * Locked with homa->grantable_lock.
  // */
  //struct list_head grantable_rpcs;

  ///**
  // * @grantable_links: Used to link this peer into homa->grantable_peers,
  // * if there are entries in grantable_rpcs. If grantable_rpcs is empty,
  // * this is an empty list pointing to itself.
  // */
  //struct list_head grantable_links;

  int outstanding_resends;
  int most_recent_resend;
  rpc_id_t least_recent_rpc;
  ap_uint<32> least_recent_ticks;
  ap_uint<32> current_ticks;
  rpc_id_t resend_rpc;
  int num_acks;
  homa_ack_t acks[NUM_PEER_UNACKED_IDS];
};

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
  ap_uint<32> buffout;
  ap_uint<32> buffin;
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
  ap_uint<128> addr;
  uint16_t dport;
  peer_id_t peer_id;
  rpc_id_t rpc_id; // TODO? When we get an incoming packet we always go through RPC table to get local ID anyway? No need to associate non local ID with the homa_rpc
  ap_uint<64> completion_cookie;
  homa_message_in_t msgin;
  homa_message_out_t msgout;
  int silent_ticks;
  ap_uint<32> resend_timer_ticks;
  ap_uint<32> done_timer_ticks;
};

/**
 * Stack for storing availible RPCs,
 */
template<typename T, int MAX_SIZE>
struct stack_t {
  T buffer[MAX_SIZE];
  int size;

  stack_t() {
    // Each RPC id begins as available 
    for (int id = 0; id < MAX_SIZE; ++id) {
      buffer[id] = id+1;
    }
    size = MAX_SIZE-1;
  }

  void push(T value) {
    /* This must be pipelined for a caller function to be pipelined */
    #pragma HLS pipeline II=1
    size++;
    buffer[size] = value;
  }

  T pop() {
    /** This must be pipelined for a caller function to be pipelined */
    #pragma HLS pipeline II=1
    T head = buffer[size];
    size--;
    return head;
  }

  bool empty() {
    return (size == 0);
  }
};

struct rpc_hashpack_t {
  ap_uint<128> s6_addr;
  uint64_t id;
  uint16_t port;
  uint16_t empty;

  bool operator==(const rpc_hashpack_t & other) const {
    return (s6_addr == other.s6_addr && id == other.id && port == other.port);
  }
};

struct peer_hashpack_t {
  ap_uint<128> s6_addr;

  bool operator==(const peer_hashpack_t & other) const {
    return (s6_addr == other.s6_addr); 
  }
};

template<typename H, typename I>
struct entry_t {
  H hashpack;
  I id;
};

template<typename H, typename I>
struct table_op_t {
  ap_uint<2> table_id;
  entry_t<H,I> entry;
};

void update_rpc_stack(hls::stream<rpc_id_t> & rpc_stack_next,
		      hls::stream<rpc_id_t> & rpc_stack_free);

void update_rpc_table(hls::stream<rpc_hashpack_t> & rpc_table_request,
		      hls::stream<rpc_id_t> & rpc_table_response,
		      hls::stream<homa_rpc_t> & rpc_table_insert);

void update_rpc_buffer(hls::stream<rpc_id_t> & rpc_buffer_request_primary,
		       hls::stream<homa_rpc_t> & rpc_buffer_response_primary,
		       hls::stream<rpc_id_t> & rpc_buffer_request_secondary,
		       hls::stream<homa_rpc_t> & rpc_buffer_response_secondary,
		       hls::stream<homa_rpc_t> & rpc_buffer_insert);

void update_peer_stack(hls::stream<peer_id_t> & peer_stack_next,
		       hls::stream<peer_id_t> & peer_stack_free);

void update_peer_table(hls::stream<peer_hashpack_t> & peer_table_request,
		       hls::stream<peer_id_t> & peer_table_response,
		       hls::stream<homa_peer_t> & peer_table_insert);

void update_peer_buffer(hls::stream<peer_id_t> & peer_buffer_request,
			hls::stream<homa_peer_t> & peer_buffer_response,
			hls::stream<homa_peer_t> & peer_buffer_insert);
 
peer_id_t homa_peer_find(in6_addr_t & addr);

uint32_t murmur3_32(const uint32_t * key, int len, uint32_t seed);
uint32_t murmur_32_scramble(uint32_t k);

#endif
