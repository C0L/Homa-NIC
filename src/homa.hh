#ifndef HOMA_H
#define HOMA_H

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

// Maximum Homa message size
#define HOMA_MAX_MESSAGE_LENGTH 1000000

#define IPV6_HEADER_LENGTH 40

#define HOMA_ETH_OVERHEAD 24
#define HOMA_MIN_PKT_LENGTH 26
#define HOMA_MAX_HEADER 90
#define ETHERNET_MAX_PAYLOAD 1500

#define HOMA_MAX_PRIORITIES 8


/*  Data "bucket" for incoming or outgoing ethernet frames
 * 
 *  Maximum size of ethernet, assuming non-jumbo frames:
 *    6*8 + 6*8 + 4*8 + 2*8 + 1500*8 + 4*8 = 12176 bits
 *
 *  axis type handles stream particulars like indicating when the stream transaction is complete
 *
 *  Refer to: https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/How-AXI4-Stream-is-Implemented
 */
typedef hls::axis<ap_uint<512>, 1, 1, 1> raw_stream_t;
//typedef hls::axis<char[64], 1, 1, 1> raw_stream_t;
typedef hls::axis<ap_uint<2048>[6], 0, 0, 0> raw_frame_t;

#define MAX_PEERS_LOG2 14

typedef ap_uint<MAX_PEERS_LOG2> peer_id_t;

// To index all the RPCs, we need LOG2 of the max number of RPCs
#define MAX_RPCS_LOG2 14
typedef ap_uint<MAX_RPCS_LOG2> rpc_id_t;

/* xmit buffer */

#define NUM_DBUFF 1024 // Number of data buffers (max outgoing RPCs)
#define DBUFF_INDEX 10 // Index into the data buffers

#define DBUFF_CHUNK_SIZE 64   // Size of a "chunk" of a data buffer
#define DBUFF_NUM_CHUNKS 256  // Number of "chunks" in one buffer
#define DBUFF_CHUNK_INDEX 8   // Index into 256 chunks

#define DBUFF_BYTE_INDEX 14

// Index into data buffers
typedef ap_uint<DBUFF_INDEX> dbuff_id_t;

typedef ap_uint<512> dbuff_chunk_t;
//struct dbuff_chunk_t {
//  char buff[DBUFF_CHUNK_SIZE];
//};

//typedef dbuff_chunk_t dbuff_t[DBUFF_NUM_CHUNKS];
//struct dbuff_t {
//  dbuff_chunk_t blocks[DBUFF_NUM_CHUNKS]; 
//};
typedef dbuff_chunk_t dbuff_t[DBUFF_NUM_CHUNKS];

typedef ap_uint<DBUFF_BYTE_INDEX> dbuff_boffset_t;
typedef ap_uint<DBUFF_CHUNK_INDEX> dbuff_coffset_t;

struct dbuff_in_t {
  dbuff_chunk_t block;
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
};

/* continuation structures */
struct out_pkt_t {
  homa_packet_type type;
  rpc_id_t rpc_id;
  dbuff_id_t dbuff_id;
  uint32_t total_bytes; // How many bytes in this packet
  uint32_t data_offset; // Next byte to load in message buffer
  uint32_t sent_bytes;  // How many bytes have been converted to chunks
  ap_uint<128> saddr;
  ap_uint<128> daddr;
  uint16_t sport;
  uint16_t dport;
  ap_uint<1> valid;
};

enum data_bytes_e {
  NO_DATA      = 0,
  ALL_DATA     = 64,
  PARTIAL_DATA = 14,
};

struct out_block_t {
  homa_packet_type type;
  dbuff_id_t dbuff_id;     // Which data buffer is the RPC message stored in
  uint32_t offset;         // Offset in data message
  data_bytes_e data_bytes; // How many data bytes to add to this block
  ap_uint<1> last;
  ap_uint<512> buff;
  //char buff[64];
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

void homa(homa_t * homa,
	  params_t * params,
	  hls::stream<raw_stream_t> & link_ingress_in, 
	  hls::stream<raw_stream_t> & link_egress_out,
	  hls::burst_maxi<dbuff_chunk_t> maxi_in, 
	  dbuff_chunk_t * maxi_out);

#endif
