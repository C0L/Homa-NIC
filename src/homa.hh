#ifndef HOMA_H
#define HOMA_H

#define DEBUG

#include "ap_int.h"
#include "hls_stream.h"
#include "hls_burst_maxi.h"
#include "ap_axi_sdata.h"

struct args_t;
struct user_output_t;

// Maximum Homa message size
#define HOMA_MAX_MESSAGE_LENGTH 1000000

// The number of RPCs supported on the device
#define MAX_RPCS 1024

// Number of bits needed to index MAX_RPCS+1 (log2 MAX_RPCS + 1)
#define MAX_RPCS_LOG2 11

#define HOMA_MAX_HEADER 90

#define ETHERNET_MAX_PAYLOAD 1500

#define IPV6_HEADER_LENGTH 40

/**
 * define HOMA_MAX_PRIORITIES - The maximum number of priority levels that
 * Homa can use (the actual number can be restricted to less than this at
 * runtime). Changing this value will affect packet formats.
 */
#define HOMA_MAX_PRIORITIES 8

/**
 * define HOMA_PEERTAB_BUCKETS - Number of bits in the bucket index for a
 * homa_peertab.  Should be large enough to hold an entry for every server
 * in a datacenter without long hash chains.
 */
#define HOMA_PEERTAB_BUCKET_BITS 15

/** define HOME_PEERTAB_BUCKETS - Number of buckets in a homa_peertab. */
#define HOMA_PEERTAB_BUCKETS (1 << HOMA_PEERTAB_BUCKET_BITS)

// To index all the RPCs, we need LOG2 of the max number of RPCs
typedef ap_uint<MAX_RPCS_LOG2> homa_rpc_id_t;

typedef ap_uint<HOMA_PEERTAB_BUCKET_BITS> homa_peer_id_t;

/** Data "bucket" for incoming or outgoing ethernet frames
 * 
 *  Maximum size of ethernet, assuming non-jumbo frames:
 *    6*8 + 6*8 + 4*8 + 2*8 + 1500*8 + 4*8 = 12176 bits
 *
 *  axis type handles stream particulars like indicating when the stream transaction is complete
 *
 *  Refer to: https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/How-AXI4-Stream-is-Implemented
 */
typedef hls::axis<unsigned char [1522], 0, 0, 0> raw_frame_t;
//typedef hls::axis<ap_uint<12176>, 0, 0, 0> raw_frame_t;

// TODO move to timer.hh
//extern ap_uint<64> timer;

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

//template<typename F, typename S>
//struct pair_t {
//  F first;
//  S second;
//};

void homa(hls::stream<raw_frame_t> & link_ingress,
	  hls::stream<raw_frame_t> & link_egress,
	  hls::stream<args_t> & user_req,
	  hls::burst_maxi<ap_uint<512>> mdma);
#endif
