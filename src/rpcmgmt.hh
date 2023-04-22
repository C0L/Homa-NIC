#ifndef RPCMGMT_H
#define RPCMGMT_H

#include "homa.hh"
#include "net.hh"
#include "xmitbuff.hh"

#define RPC_SUB_TABLE_SIZE 16384
#define RPC_SUB_TABLE_INDEX 14

#define SEED0 0x7BF6BF21
#define SEED1 0x9FA91FE9
#define SEED2 0xD0C8FBDF
#define SEED3 0xE6D0851C

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
  homa_peer_id_t peer;
  in6_addr_t addr;
  ap_uint<16> dport;
  homa_rpc_id_t id; // TODO? When we get an incoming packet we always go through RPC table to get local ID anyway? No need to associate non local ID with the homa_rpc
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
struct homa_rpc_stack_t {
  homa_rpc_id_t buffer[MAX_RPCS];
  int size;

  homa_rpc_stack_t() {
#pragma HLS ARRAY_PARTITION variable=buffer type=complete
    // Each RPC id begins as available 
    for (int id = 0; id < MAX_RPCS; ++id) {
      buffer[id] = id+1;
    }
    size = MAX_RPCS;
  }

  void push(homa_rpc_id_t rpc_id) {
#pragma HLS ARRAY_PARTITION variable=buffer type=complete
    /** This must be pipelined for a caller function to be pipelined */
#pragma HLS PIPELINE
    // A shift register is inferred
    for (int id = MAX_RPCS-1; id > 0; --id) {
    #pragma HLS unroll
      buffer[id] = buffer[id-1];
    }

    buffer[0] = rpc_id;
    size++;
  }

  homa_rpc_id_t pop() {
#pragma HLS ARRAY_PARTITION variable=buffer type=complete
    /** This must be pipelined for a caller function to be pipelined */
#pragma HLS PIPELINE
    homa_rpc_id_t head = buffer[0];
    // A shift register is inferred
    for (int id = 0; id < MAX_RPCS; ++id) {
    #pragma HLS unroll
      buffer[id] = buffer[id+1];
    }

    size--;
    return head;
  }

  int get_size() {
    return size;
  }

  bool empty() {
    return (size == 0);
  }
};

struct hashpack_t {
  ap_uint<128> s6_addr;
  uint64_t id;
  uint16_t port;
  uint16_t empty;
};

struct homa_rpc_entry_t {
  hashpack_t hashpack; 
  homa_rpc_id_t homa_rpc;
};

struct rpc_table_op_t {
  ap_uint<2> table_id;
  homa_rpc_entry_t entry;
};

void update_rpc_stack(hls::stream<homa_rpc_id_t> freed_rpcs, hls::stream<homa_rpc_id_t> new_rpcs);

// homa_rpc_t & homa_rpc_new_client(homa_t * homa, ap_uint<32> & buffout, ap_uint<32> & buffin, in6_addr_t & dest_addr);
// homa_rpc_t & homa_rpc_find(ap_uint<64> id);
// void homa_insert_server_rpc(sockaddr_in6_t & addr, uint64_t & id, homa_rpc_id_t & homa_rpc_id);
// homa_rpc_id_t homa_find_server_rpc(sockaddr_in6_t & addr, uint64_t & id);
// void rpc_server_insert(hashpack_t & hashpack, homa_rpc_id_t & homa_rpc_id);
// homa_rpc_id_t rpc_server_search(hashpack_t & pack);

uint32_t murmur3_32(const uint32_t * key, int len, uint32_t seed);
uint32_t murmur_32_scramble(uint32_t k);

#endif
