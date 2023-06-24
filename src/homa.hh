#ifndef HOMA_H
#define HOMA_H

#include <iostream>

#define DEBUG

#include "ap_int.h"
#include "hls_stream.h"
#include "hls_burst_maxi.h"
#include "ap_axi_sdata.h"

struct args_t;
struct user_output_t;

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

// Client RPC IDs are even, servers are odd 
#define IS_CLIENT(id) ((id & 1) == 0)

// Sender->Client and Client->Sender rpc ID conversion
#define LOCALIZE_ID(sender_id) ((sender_id) ^ 1);

// #define DEBUG_MSG(a) std::cerr << "DEBUG: " << a << std::endl;
#define DEBUG_MSG(a)

#ifndef DEBUG 
#define MAX_SRPT 1024
#else
#define MAX_SRPT 8
#endif

#ifndef DEBUG 
#define MAX_OVERCOMMIT 1
#else
#define MAX_OVERCOMMIT 1
#endif

#define SRPT_UPDATE 0
#define SRPT_UPDATE_BLOCK 1
#define SRPT_INVALIDATE 2
#define SRPT_UNBLOCK 3
#define SRPT_EMPTY 4
#define SRPT_BLOCKED 5
#define SRPT_ACTIVE 6

// Maximum Homa message size
#define HOMA_MAX_MESSAGE_LENGTH 1000000

#define IPV6_HEADER_LENGTH 40

#define HOMA_ETH_OVERHEAD 24
#define HOMA_MIN_PKT_LENGTH 26
#define HOMA_MAX_HEADER 90
#define ETHERNET_MAX_PAYLOAD 1500

#define HOMA_MAX_PRIORITIES 8

// How many bytes need to be accumulated before another GRANT pkt is sent
#define GRANT_REFRESH 10000

#define MAX_PEERS_LOG2 14

typedef ap_uint<MAX_PEERS_LOG2> peer_id_t;

// To index all the RPCs, we need LOG2 of the max number of RPCs
#define MAX_RPCS_LOG2 14
typedef ap_uint<MAX_RPCS_LOG2> rpc_id_t;

/* data buffer */

#define NUM_DBUFF 1024 // Number of data buffers (max outgoing RPCs)
#define DBUFF_INDEX 10 // Index into the data buffers

#define DBUFF_CHUNK_SIZE 64   // Size of a "chunk" of a data buffer
#define DBUFF_NUM_CHUNKS 256  // Number of "chunks" in one buffer
#define DBUFF_CHUNK_INDEX 8   // Index into 256 chunks

#define DBUFF_BYTE_INDEX 14

// Index into data buffers
typedef ap_uint<DBUFF_INDEX> dbuff_id_t;

struct integral_t {
  //unsigned char data[64];
  ap_uint<512> data;
};

typedef integral_t dbuff_t[DBUFF_NUM_CHUNKS];

typedef ap_uint<DBUFF_BYTE_INDEX> dbuff_boffset_t;
typedef ap_uint<DBUFF_CHUNK_INDEX> dbuff_coffset_t;

struct dbuff_in_t {
  integral_t block;
  dbuff_id_t dbuff_id;
  dbuff_coffset_t dbuff_chunk;
};

struct dbuff_notif_t {
  dbuff_id_t dbuff_id;
  dbuff_coffset_t dbuff_chunk; 
};

typedef hls::axis<integral_t, 1, 1, 1> raw_stream_t;

/* homa structures */
//enum homa_packet_type {
//  DATA               = 0x10,
//  GRANT              = 0x11,
//  RESEND             = 0x12,
//  UNKNOWN            = 0x13,
//  BUSY               = 0x14,
//  CUTOFFS            = 0x15,
//  FREEZE             = 0x16,
//  NEED_ACK           = 0x17,
//  ACK                = 0x18,
//};

#define DATA     0x10
#define GRANT    0x11
#define RESEND   0x12
#define UNKNOWN  0x13
#define BUSY     0x14
#define CUTOFFS  0x15
#define FREEZE   0x16
#define NEED_ACK 0x17
#define ACK      0x18

typedef ap_uint<8> homa_packet_type;

/**
 * struct homa - Overall information about the Homa protocol implementation.
 */
struct homa_t {
  ap_int<32> mtu;
  ap_uint<32> rtt_bytes;
  ap_int<32> link_mbps;
  ap_int<32> num_priorities;
  ap_int<32> priority_map[HOMA_MAX_PRIORITIES];
  ap_int<32> max_sched_prio;
  ap_int<32> unsched_cutoffs[HOMA_MAX_PRIORITIES];
  ap_int<32> cutoff_version;
  ap_int<32> duty_cycle;
  ap_int<32> grant_threshold;
  ap_int<32> max_overcommit;
  ap_int<32> max_incoming;
  ap_int<32> resend_ticks;
  ap_int<32> resend_interval;
  ap_int<32> timeout_resends;
  ap_int<32> request_ack_ticks;
  ap_int<32> reap_limit;
  ap_int<32> dead_buffs_limit;
  ap_int<32> max_dead_buffs;
  ap_uint<32> timer_ticks;
  ap_int<32> flags;
};

/**
 * struct homa_message_out - Describes a message (either request or response)
 * for which this machine is the sender.
 */
struct homa_message_out_t {
  ap_int<32> length;
  dbuff_id_t dbuff_id;
  ap_int<32> unscheduled;
  ap_int<32> granted;
  //ap_uint<8> sched_priority;
  //ap_uint<64> init_cycles;
};

/**
 * struct homa_message_in - Holds the state of a message received by
 * this machine; used for both requests and responses.
 */
struct homa_message_in_t {
  ap_int<32> total_length;
  ap_int<32> bytes_remaining;
  ap_int<32> incoming;
  ap_int<32> priority;
  bool scheduled;
  ap_uint<64> birth;
  ap_int<64> copied_out;
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

  //int flags;
  //int grants_in_progress;
  ap_uint<128> saddr;
  ap_uint<128> daddr;
  ap_uint<16> dport;
  ap_uint<16> sport;
  peer_id_t peer_id;
  rpc_id_t rpc_id; 
  ap_uint<64> completion_cookie;
  homa_message_in_t msgin;
  homa_message_out_t msgout;

  //uint32_t rtt_bytes;
  //int silent_ticks;
  //ap_uint<32> resend_timer_ticks;
  //ap_uint<32> done_timer_ticks;
};

struct sendmsg_t {
  ap_uint<32> buffin; // Offset in DMA space for input
  ap_uint<32> length; // Total length of message
  
  ap_uint<128> saddr; // Sender address
  ap_uint<128> daddr; // Destination address

  ap_uint<16> sport; // Sender port
  ap_uint<16> dport; // Destination port

  ap_uint<64> id; // RPC specified by caller
  ap_uint<64> completion_cookie;

  ap_uint<1> valid;

  // Internal use
  ap_uint<32> granted;
  rpc_id_t local_id; // Local RPC ID 
  dbuff_id_t dbuff_id; // Data buffer ID for outgoing data
  peer_id_t peer_id; // Local ID for this destination address
};

struct recvmsg_t {
  ap_uint<32> buffout; // Offset in DMA space for output

  ap_uint<128> saddr; // Sender address
  ap_uint<128> daddr; // Destination address

  ap_uint<16> sport; // Sender port
  ap_uint<16> dport; // Destination port

  ap_uint<64> id; // RPC specified by caller

  ap_uint<1> valid;

  // Internal use
  rpc_id_t local_id; // Local RPC ID 
  peer_id_t peer_id; // Local ID for this peer
};

/* continuation structures */

// TODO switch all of these to ap_uints (big endian). Perform conversion on head.
struct header_t {
  // Local Values
  rpc_id_t local_id;
  peer_id_t peer_id;
  dbuff_id_t dbuff_id;
  ap_uint<32> dma_offset;
  ap_uint<32> processed_bytes;
  ap_uint<32> packet_bytes;
  ap_uint<32> rtt_bytes;

  // IPv6 + Common Header
  ap_uint<16> payload_length;
  ap_uint<128> saddr;
  ap_uint<128> daddr;
  ap_uint<16> sport;
  ap_uint<16> dport;
  homa_packet_type type;
  ap_uint<64> sender_id;

  // Data Header
  ap_uint<32> message_length; 
  ap_uint<32> incoming;
  ap_uint<16> cutoff_version;
  ap_uint<8> retransmit;

  // Data Segment
  ap_uint<32> data_offset;
  ap_uint<32> segment_length;

  // Ack Header
  ap_uint<64> client_id;
  ap_uint<16> client_port;
  ap_uint<16> server_port;

  // Grant Header
  ap_uint<32> grant_offset;
  ap_uint<8> priority;

  ap_uint<1> valid;
};

struct ready_grant_pkt_t {
  peer_id_t peer_id;
  rpc_id_t rpc_id;
  ap_uint<32> grantable;
};

struct ready_data_pkt_t {
  rpc_id_t rpc_id;
  dbuff_id_t dbuff_id;
  ap_uint<32> remaining;
  ap_uint<32> total;
  ap_uint<32> granted;
};

enum data_bytes_e {
  NO_DATA      = 0,
  ALL_DATA     = 64,
  PARTIAL_DATA = 14,
};

struct in_chunk_t {
  integral_t buff;
  ap_uint<32> offset;
  ap_uint<1> last;
};

struct out_chunk_t {
  homa_packet_type type;
  dbuff_id_t dbuff_id;     // Which data buffer is the RPC message stored in
  ap_uint<32> offset;      // Offset in data message
  data_bytes_e data_bytes; // How many data bytes to add to this block
  ap_uint<1> last;
  integral_t buff;
};

struct dma_r_req_t {
  ap_uint<32> offset;
  ap_uint<32> length;
  dbuff_id_t dbuff_id;
};

struct dma_w_req_t {
  ap_uint<32> offset;
  integral_t block;
};


 
const ap_uint<16> ETHERTYPE_IPV6 = 0x86DD;
const ap_uint<48> MAC_DST = 0xAAAAAAAAAAAA;
const ap_uint<48> MAC_SRC = 0xBBBBBBBBBBBB;
const ap_uint<32> VTF = 0x600FFFFF;
const ap_uint<8> IPPROTO_HOMA = 0xFD;
const ap_uint<8> HOP_LIMIT = 0x00;
const ap_uint<8> DOFF = 10 << 4;
const ap_uint<8> DATA_TYPE = DATA;
const ap_uint<32> HOMA_PAYLOAD_SIZE = 1386;


struct srpt_data_t {
  rpc_id_t rpc_id;
  dbuff_id_t dbuff_id;
  ap_uint<32> remaining;
  ap_uint<32> total;

  srpt_data_t() {
    rpc_id = 0;
    dbuff_id = 0;
    remaining = 0xFFFFFFFF;
    total = 0;
  }

  srpt_data_t(rpc_id_t rpc_id, dbuff_id_t dbuff_id, ap_uint<32> remaining, ap_uint<32> total) {
    this->rpc_id = rpc_id;
    this->dbuff_id = dbuff_id;
    this->remaining = remaining;
    this->total = total;
  }

  bool operator==(const srpt_data_t & other) const {
    return (rpc_id == other.rpc_id &&
  	    remaining == other.remaining &&
  	    total == other.total);
  }

  // Ordering operator
  bool operator>(srpt_data_t & other) {
    return remaining > other.remaining;
  }

  void update_priority(srpt_data_t & other) {}

  void print() {
    std::cerr << rpc_id << " " << remaining << std::endl;
  }
};

struct srpt_grant_t {
  peer_id_t peer_id;
  rpc_id_t rpc_id;
  ap_uint<32> grantable;
  ap_uint<3> priority;

  srpt_grant_t() {
    peer_id = 0;
    rpc_id = 0;
    grantable = 0xFFFFFFFF;
    priority = SRPT_EMPTY;
  }

  srpt_grant_t(peer_id_t peer_id, rpc_id_t rpc_id, ap_uint<32> grantable, ap_uint<3> priority) {
    this->peer_id = peer_id;
    this->rpc_id = rpc_id;
    this->grantable = grantable;
    this->priority = priority;
  }

  bool operator==(const srpt_grant_t & other) const {
    return (peer_id == other.peer_id &&
  	    rpc_id == other.rpc_id &&
  	    grantable == other.grantable &&
  	    priority == other.priority);
  }

  // Ordering operator
  bool operator>(srpt_grant_t & other) {
    return (priority != other.priority) ? (priority < other.priority) : grantable > other.grantable;
  }

  void update_priority(srpt_grant_t & other) {

    if (other.peer_id == peer_id && other.rpc_id == rpc_id && priority == SRPT_UPDATE_BLOCK) {
      other.grantable = grantable;
      other.priority = SRPT_BLOCKED;
    } else if (other.peer_id == peer_id && priority == SRPT_UPDATE_BLOCK) {
      other.priority = SRPT_BLOCKED;
    } else if (other.peer_id == peer_id && other.rpc_id == rpc_id && priority == SRPT_UPDATE) {
      other.grantable = grantable;
    } else if (other.peer_id == peer_id && priority == SRPT_UNBLOCK) {
      other.priority = SRPT_ACTIVE;
    } else if (other.peer_id == peer_id && other.rpc_id == rpc_id && priority == SRPT_INVALIDATE) {
      other.priority = SRPT_INVALIDATE;
    }
  }

  void print() {
    std::cerr << peer_id << " " << grantable << " " << priority << " " << rpc_id << std::endl;
  }
};


template<typename T, int FIFO_SIZE>
struct fifo_t {
  T buffer[FIFO_SIZE];

  int read_head;

  fifo_t() {
    read_head = -1;
  }

  void insert(T value) {
#pragma HLS array_partition variable=buffer type=complete

    for (int i = FIFO_SIZE-2; i >= 0; --i) {
#pragma HLS unroll
      buffer[i+1] = buffer[i];
    }
    
    buffer[0] = value;
    
    read_head++;
  }

  T remove() {
#pragma HLS array_partition variable=buffer type=complete
    T val = buffer[read_head];

    read_head--;
    return val;
  }

  void dump() {
    for (int i = 0; i < 64; ++i) {
      std::cerr << buffer[i].offset << std::endl;
    }
  }

  T & head() {
    return buffer[read_head];
  }

  bool full() {
    return read_head == FIFO_SIZE-1;
  }

  bool empty() {
    return read_head == -1;
  }
};


template<typename T, int FIFO_SIZE>
struct fifo_id_t {
  T buffer[FIFO_SIZE];

  int read_head;

  fifo_id_t(bool init) {
    if (init) {
      // Each RPC id begins as available 
      for (int id = 0; id < FIFO_SIZE; ++id) {
	buffer[id] = id;
      }
      read_head = FIFO_SIZE-1;
    } else {
      read_head = -1;
    }
  }

  void insert(T value) {
#pragma HLS array_partition variable=buffer type=complete

    for (int i = FIFO_SIZE-2; i >= 0; --i) {
#pragma HLS unroll
      buffer[i+1] = buffer[i];
    }
    
    buffer[0] = value;
    
    read_head++;
  }

  T remove() {
#pragma HLS array_partition variable=buffer type=complete
    T val = buffer[read_head];

    read_head--;
    return val;
  }

  T & head() {
    return buffer[read_head];
  }

  bool empty() {
    return read_head == -1;
  }
};

template<int W>
void htno_set(ap_uint<W*8> & out, const ap_uint<W*8> & in) {
#pragma HLS inline 
  for (int i = 0; i < W; ++i) {
#pragma HLS unroll
    out((W * 8) - 1 - (i * 8), (W*8) - 8 - (i * 8)) = in(7 + (i * 8), 0 + (i * 8));
  }
}



void homa(homa_t * homa,
	  sendmsg_t * sendmsg,
	  recvmsg_t * recvmsg,
	  hls::stream<raw_stream_t> & link_ingress_in, 
	  hls::stream<raw_stream_t> & link_egress_out,
	  char * maxi_in,
	  char * maxi_out);

#endif
