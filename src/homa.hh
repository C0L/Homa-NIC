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

//TODO rename?
//typedef raw_frame unsigned char[1522];

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
extern ap_uint<64> timer;

/**
 * struct homa - Overall information about the Homa protocol implementation.
 *
 * There will typically only exist one of these at a time, except during
 * unit tests.
 */
struct homa_t {
  int mtu;

  /**
   * @rtt_bytes: An estimate of the amount of data that can be transmitted
   * over the wire in the time it takes to send a full-size data packet
   * and receive back a grant. Used to ensure full utilization of
   * uplink bandwidth. Set externally via sysctl.
   */
  int rtt_bytes;

  /**
   * @link_bandwidth: The raw bandwidth of the network uplink, in
   * units of 1e06 bits per second.  Set externally via sysctl.
   */
  int link_mbps;

  /**
   * @num_priorities: The total number of priority levels available for
   * Homa's use. Internally, Homa will use priorities from 0 to
   * num_priorities-1, inclusive. Set externally via sysctl.
   */
  int num_priorities;

  /**
   * @priority_map: entry i gives the value to store in the high-order
   * 3 bits of the DSCP field of IP headers to implement priority level
   * i. Set externally via sysctl.
   */
  int priority_map[HOMA_MAX_PRIORITIES];

  /**
   * @max_sched_prio: The highest priority level currently available for
   * scheduled packets. Levels above this are reserved for unscheduled
   * packets.  Set externally via sysctl.
   */
  int max_sched_prio;

  /**
   * @unsched_cutoffs: the current priority assignments for incoming
   * unscheduled packets. The value of entry i is the largest
   * message size that uses priority i (larger i is higher priority).
   * If entry i has a value of HOMA_MAX_MESSAGE_SIZE or greater, then
   * priority levels less than i will not be used for unscheduled
   * packets. At least one entry in the array must have a value of
   * HOMA_MAX_MESSAGE_SIZE or greater (entry 0 is usually INT_MAX).
   * Set externally via sysctl.
   */
  int unsched_cutoffs[HOMA_MAX_PRIORITIES];

  /**
   * @cutoff_version: increments every time unsched_cutoffs is
   * modified. Used to determine when we need to send updates to
   * peers.  Note: 16 bits should be fine for this: the worst
   * that happens is a peer has a super-stale value that equals
   * our current value, so the peer uses suboptimal cutoffs until the
   * next version change.  Can be set externally via sysctl.
   */
  int cutoff_version;

  /**
   * @duty_cycle: Sets a limit on the fraction of network bandwidth that
   * may be consumed by a single RPC in units of one-thousandth (1000
   * means a single RPC can consume all of the incoming network
   * bandwidth, 500 means half, and so on). This also determines the
   * fraction of a core that can be consumed by NAPI when a large
   * message is being received. Its main purpose is to keep NAPI from
   * monopolizing a core so much that user threads starve. Set externally
   * via sysctl.
   */
  int duty_cycle;

  /**
   * @grant_threshold: A grant will not be sent for an RPC until
   * the number of incoming bytes drops below this threshold. Computed
   * from @rtt_bytes and @duty_cycle.
   */
  int grant_threshold;

  /**
   * @max_overcommit: The maximum number of messages to which Homa will
   * send grants at any given point in time.  Set externally via sysctl.
   */
  int max_overcommit;

  /**
   * @max_incoming: This value is computed from max_overcommit, and
   * is the limit on how many bytes are currently permitted to be
   * granted but not yet received, cumulative across all messages.
   */
  int max_incoming;

  /**
   * @resend_ticks: When an RPC's @silent_ticks reaches this value,
   * start sending RESEND requests.
   */
  int resend_ticks;

  /**
   * @resend_interval: minimum number of homa timer ticks between
   * RESENDs to the same peer.
   */
  int resend_interval;

  /**
   * @timeout_resends: Assume that a server is dead if it has not
   * responded after this many RESENDs have been sent to it.
   */
  int timeout_resends;

  /**
   * @request_ack_ticks: How many timer ticks we'll wait for the
   * client to ack an RPC before explicitly requesting an ack.
   * Set externally via sysctl.
   */
  int request_ack_ticks;

  /**
   * @reap_limit: Maximum number of packet buffers to free in a
   * single call to home_rpc_reap.
   */
  int reap_limit;

  /**
   * @dead_buffs_limit: If the number of packet buffers in dead but
   * not yet reaped RPCs is less than this number, then Homa reaps
   * RPCs in a way that minimizes impact on performance but may permit
   * dead RPCs to accumulate. If the number of dead packet buffers
   * exceeds this value, then Homa switches to a more aggressive approach
   * to reaping RPCs. Set externally via sysctl.
   */
  int dead_buffs_limit;

  /**
   * @max_dead_buffs: The largest aggregate number of packet buffers
   * in dead (but not yet reaped) RPCs that has existed so far in a
   * single socket.  Readable via sysctl, and may be reset via sysctl
   * to begin recalculating.
   */
  int max_dead_buffs;

  /**
   * @timer_ticks: number of times that homa_timer has been invoked
   * (may wraparound, which is safe).
   */
  ap_uint<32> timer_ticks;

  /**
   * @flags: a collection of bits that can be set using sysctl
   * to trigger various behaviors.
   */
  int flags;
};

void homa(hls::stream<raw_frame_t> & link_ingress,
	  hls::stream<raw_frame_t> & link_egress,
	  hls::stream<args_t> & user_req,
	  hls::burst_maxi<ap_uint<512>> mdma);
	  //char * mdma);
	  //ap_uint<512> * dma);
#endif
