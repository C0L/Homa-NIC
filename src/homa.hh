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
#define MAX_SRPT 32
#endif

#define MAX_OVERCOMMIT 8

#define SRPT_MSG 0
#define SRPT_EMPTY 1
#define SRPT_BLOCKED 2
#define SRPT_ACTIVE 3

// Maximum Homa message size
#define HOMA_MAX_MESSAGE_LENGTH 1000000

#define IPV6_HEADER_LENGTH 40

#define HOMA_ETH_OVERHEAD 24
#define HOMA_MIN_PKT_LENGTH 26
#define HOMA_MAX_HEADER 90
#define ETHERNET_MAX_PAYLOAD 1500

#define HOMA_MAX_PRIORITIES 8

typedef hls::axis<ap_uint<512>, 1, 1, 1> raw_stream_t;
// typedef hls::axis<ap_uint<2048>[6], 0, 0, 0> raw_frame_t;

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

typedef ap_uint<512> dbuff_chunk_t;

typedef dbuff_chunk_t dbuff_t[DBUFF_NUM_CHUNKS];

typedef ap_uint<DBUFF_BYTE_INDEX> dbuff_boffset_t;
typedef ap_uint<DBUFF_CHUNK_INDEX> dbuff_coffset_t;

struct dbuff_in_t {
  dbuff_chunk_t block;
  dbuff_id_t dbuff_id;
  dbuff_coffset_t dbuff_chunk;
};

struct dbuff_notif_t {
  dbuff_id_t dbuff_id;
  dbuff_coffset_t dbuff_chunk; 
};

/* network structures */
typedef uint16_t sa_family_t;
typedef uint16_t in_port_t;

struct sockaddr_t {
  sa_family_t      sa_family;
  unsigned char    sa_data[14];
};

struct in_addr_t {
  uint32_t       s_addr;     /* address in network byte order */
};

struct sockaddr_in_t {
  sa_family_t    sin_family; /* address family: AF_INET */
  in_port_t      sin_port;   /* port in network byte order */
  in_addr_t      sin_addr;   /* internet address */
};

struct in6_addr_t {
  ap_uint<128>    s6_addr;   /* IPv6 address */
};

struct sockaddr_in6_t {
  sa_family_t     sin6_family;   /* AF_INET6 */
  in_port_t       sin6_port;     /* port number */
  uint32_t        sin6_flowinfo; /* IPv6 flow information */
  in6_addr_t      sin6_addr;     /* IPv6 address */
  uint32_t        sin6_scope_id; /* Scope ID (new in 2.4) */
};

typedef union sockaddr_in_union {
  sockaddr_t sa;
  sockaddr_in_t in4;
  sockaddr_in6_t in6;
} sockaddr_in_union_t;


/* homa structures */
enum homa_packet_type {
  DATA               = 0x10,
  GRANT              = 0x11,
  RESEND             = 0x12,
  UNKNOWN            = 0x13,
  BUSY               = 0x14,
  CUTOFFS            = 0x15,
  FREEZE             = 0x16,
  NEED_ACK           = 0x17,
  ACK                = 0x18,
};

/**
 * struct homa - Overall information about the Homa protocol implementation.
 */
struct homa_t {
  int mtu;
  int rtt_bytes;
  int link_mbps;
  int num_priorities;
  int priority_map[HOMA_MAX_PRIORITIES];
  int max_sched_prio;
  int unsched_cutoffs[HOMA_MAX_PRIORITIES];
  int cutoff_version;
  int duty_cycle;
  int grant_threshold;
  int max_overcommit;
  int max_incoming;
  int resend_ticks;
  int resend_interval;
  int timeout_resends;
  int request_ack_ticks;
  int reap_limit;
  int dead_buffs_limit;
  int max_dead_buffs;
  ap_uint<32> timer_ticks;
  int flags;
};

struct params_t {
  // Offset in DMA space for output
  uint32_t buffout;
  // Offset in DMA space for input
  uint32_t buffin;
  uint32_t length;
  sockaddr_in6_t dest_addr;
  sockaddr_in6_t src_addr;
  uint64_t id;
  uint64_t completion_cookie;
  ap_uint<1> valid;
};

/* continuation structures */
struct header_out_t {
  homa_packet_type type;
  rpc_id_t rpc_id;
  uint32_t dma_offset;
  dbuff_id_t dbuff_id;
  uint32_t total_bytes;   // How many bytes in this message
  uint32_t packet_bytes;  // How many bytes in this packet 
  uint32_t data_offset;   // Next byte to load in message buffer
  uint32_t sent_bytes;    // How many bytes have been converted to chunks
  uint32_t granted_bytes; // Offset in message that we can send up to
  uint32_t msg_length;    // Total length of the message
  ap_uint<128> saddr;
  ap_uint<128> daddr;
  uint16_t sport;
  uint16_t dport;
  ap_uint<1> valid;
};

struct header_in_t {
  homa_packet_type type;
  rpc_id_t rpc_id;         // Internal RPC ID
  uint64_t id;             // RPC ID from the packet
  peer_id_t peer_id;
  uint32_t dma_offset;
  uint32_t total_bytes;   // How many bytes in this message
  uint32_t packet_bytes;  // How many bytes in this packet 
  uint32_t data_offset;   // Next byte to load in message buffer
  uint32_t read_bytes;    // How many bytes have been converted from chunks
  uint32_t granted_bytes; // Offset of newly recieved grant
  uint32_t msg_length;    // Total length of the message
  ap_uint<128> saddr;
  ap_uint<128> daddr;
  uint16_t sport;
  uint16_t dport;
  ap_uint<1> valid;
};

struct ready_grant_pkt_t {
  rpc_id_t rpc_id;
  uint32_t granted;
  // TODO 
};

struct ready_data_pkt_t {
  rpc_id_t rpc_id;
  dbuff_id_t dbuff_id;
  uint32_t remaining;
  uint32_t total;
  uint32_t granted;
};

enum data_bytes_e {
  NO_DATA      = 0,
  ALL_DATA     = 64,
  PARTIAL_DATA = 14,
};

struct in_chunk_t {
  dbuff_chunk_t buff;
  uint32_t offset;
  ap_uint<1> last;
};

struct out_chunk_t {
  homa_packet_type type;
  dbuff_id_t dbuff_id;     // Which data buffer is the RPC message stored in
  uint32_t offset;         // Offset in data message
  data_bytes_e data_bytes; // How many data bytes to add to this block
  ap_uint<1> last;
  ap_uint<512> buff;
};

struct new_rpc_t {
  dbuff_id_t dbuff_id;
  uint32_t buffout;
  uint32_t buffin;
  uint32_t rtt_bytes;
  uint32_t length;
  uint32_t granted;
  sockaddr_in6_t dest_addr;
  sockaddr_in6_t src_addr;
  uint64_t id;
  rpc_id_t local_id;
  peer_id_t peer_id;
  uint64_t completion_cookie;
};


struct dma_r_req_t {
  uint32_t offset;
  dbuff_id_t dbuff_id;
};

struct dma_w_req_t {
  uint32_t offset;
  dbuff_chunk_t block;
};



struct srpt_data_t {
  rpc_id_t rpc_id;
  dbuff_id_t dbuff_id;
  uint32_t remaining;
  uint32_t total;

  srpt_data_t() {
    rpc_id = 0;
    dbuff_id = 0;
    remaining = 0xFFFFFFFF;
    total = 0;
  }

  //data TODO total probably does not need to be stored
  srpt_data_t(rpc_id_t rpc_id, dbuff_id_t dbuff_id, uint32_t remaining, uint32_t total) {
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
  uint32_t grantable;
  ap_uint<2> priority;

  srpt_grant_t() {
    peer_id = 0;
    rpc_id = 0;
    grantable = 0xFFFFFFFF;
    priority = SRPT_EMPTY;
  }

  srpt_grant_t(peer_id_t peer_id, uint32_t grantable,  rpc_id_t rpc_id, ap_uint<2> priority) {
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
    if (priority == SRPT_MSG && peer_id == other.peer_id) {
      return true;
    } else {
      return (priority != other.priority) ? (priority < other.priority) : grantable > other.grantable;
    } 
  }

  void update_priority(srpt_grant_t & other) {
    if (priority == SRPT_MSG && other.peer_id == peer_id) {
      other.priority = (other.priority == SRPT_BLOCKED) ? SRPT_ACTIVE : SRPT_BLOCKED;
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
    for (int i = FIFO_SIZE-2; i > 0; --i) {
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

  bool full() {
    return read_head == MAX_SRPT-1;
  }

  bool empty() {
    return read_head == -1;
  }
};



void homa(homa_t * homa,
	  params_t * params,
	  hls::stream<raw_stream_t> & link_ingress_in, 
	  hls::stream<raw_stream_t> & link_egress_out,
	  dbuff_chunk_t * maxi_in, 
	  dbuff_chunk_t * maxi_out);

#endif
