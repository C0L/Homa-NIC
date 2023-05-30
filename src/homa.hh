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

// Maximum Homa message size
#define HOMA_MAX_MESSAGE_LENGTH 1000000

#define IPV6_HEADER_LENGTH 40

#define HOMA_ETH_OVERHEAD 24
#define HOMA_MIN_PKT_LENGTH 26
#define HOMA_MAX_HEADER 90
#define ETHERNET_MAX_PAYLOAD 1500

#define HOMA_MAX_PRIORITIES 8

#define NUM_XMIT_BUFFER_INDEX 10
#define MAX_RPCS_LOG2 14

/*  Data "bucket" for incoming or outgoing ethernet frames
 * 
 *  Maximum size of ethernet, assuming non-jumbo frames:
 *    6*8 + 6*8 + 4*8 + 2*8 + 1500*8 + 4*8 = 12176 bits
 *
 *  axis type handles stream particulars like indicating when the stream transaction is complete
 *
 *  Refer to: https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/How-AXI4-Stream-is-Implemented
 */
typedef hls::axis<char[64], 1, 1, 1> raw_stream_t;
//typedef hls::axis<ap_uint<512>, 1, 1, 1> raw_stream_t;
typedef hls::axis<ap_uint<2048>[6], 0, 0, 0> raw_frame_t;

// Index into xmit buffers
typedef ap_uint<NUM_XMIT_BUFFER_INDEX> xmit_id_t;

// To index all the RPCs, we need LOG2 of the max number of RPCs
typedef ap_uint<MAX_RPCS_LOG2> rpc_id_t;

//typedef char packet_t[1522];

struct packet_t {
  char buff[1522];
};

//struct data_pkt_cont_t {
//  char buff[64];
//  rpc_id_t rpc_id;
//  ap_uint<128> addr;
//  uint16_t dport;
//  xmit_id_t xmit_id;
//  uint32_t remaining_bytes;
//};

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


struct pending_pkt_t {
  homa_packet_type type;
  rpc_id_t rpc_id;
  xmit_id_t xmit_id;
  uint32_t total_bytes;
  uint32_t data_bytes;
  uint32_t sent_bytes;
  ap_uint<128> saddr;
  ap_uint<128> daddr;
  uint16_t sport;
  uint16_t dport;
  ap_uint<1> valid;
};

struct pkt_block_t {
  homa_packet_type type;
  xmit_id_t xmit_id;
  uint8_t data_bytes;
  ap_uint<1> done;
  char buff[64];
};

void homa(hls::stream<raw_frame_t> & link_ingress,
	  hls::stream<raw_stream_t> & link_egress,
	  hls::stream<args_t> & user_req,
	  hls::burst_maxi<ap_uint<512>> mdma);

#endif
