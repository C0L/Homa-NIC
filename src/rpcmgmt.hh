#ifndef RPCMGMT_H
#define RPCMGMT_H

#include "homa.hh"
#include "net.hh"

// Roughly 10 ethernet packets worth of message space (scale this based on DDR latency)
#define HOMA_MESSAGE_CACHE 15000

/**
 * struct homa_message_out - Describes a message (either request or response)
 * for which this machine is the sender.
 */
struct homa_message_out_t {
  /**
   * @length: Total bytes in message (excluding headers).  A value
   * less than 0 means this structure is uninitialized and therefore
   * not in use (all other fields will be zero in this case).
   */
  int length;

  /**
   * @message: On-chip cache of message ready to broadcast
   */
  //char message[HOMA_MESSAGE_CACHE];
  ap_uint<512> message[235];

  /**
   * @next_xmit_offset: All bytes in the message, up to but not
   * including this one, have been transmitted.
   */
  int next_xmit_offset;

  /**
   * @unscheduled: Initial bytes of message that we'll send
   * without waiting for grants.
   */
  int unscheduled;

  /**
   * @granted: Total number of bytes we are currently permitted to
   * send, including unscheduled bytes; must wait for grants before
   * sending bytes at or beyond this position. Never larger than
   * @length.
   */
  int granted;

  /** @priority: Priority level to use for future scheduled packets. */
  ap_uint<8> sched_priority;

  /**
   * @init_cycles: Time in get_cycles units when this structure was
   * initialized.  Used to find the oldest outgoing message.
   */
  ap_uint<64> init_cycles;
};

/**
 * struct homa_message_in - Holds the state of a message received by
 * this machine; used for both requests and responses.
 */
struct homa_message_in_t {
  /**
   * @total_length: Size of the entire message, in bytes. A value
   * less than 0 means this structure is uninitialized and therefore
   * not in use.
   */
  int total_length;
  
  /**
   * @dma_out: Userspace DMA buffer for placing received message
   */
  //user_output_t * dma_out;
  // TODO Switch to DMA offset?
  // TODO This is already in homa_rpc_t??
  
  /**
   * @bytes_remaining: Amount of data for this message that has
   * not yet been received; will determine the message's priority.
   */
  int bytes_remaining;
  
  /**
   * @incoming: Total # of bytes of the message that the sender will
   * transmit without additional grants. Initialized to the number of
   * unscheduled bytes; after that, updated only when grants are sent.
   * Never larger than @total_length. 
   */
  int incoming;
  
  /** @priority: Priority level to include in future GRANTS. */
  int priority;
  
  /**
   * @scheduled: True means some of the bytes of this message
   * must be scheduled with grants.
   */
  bool scheduled;
  
  /**
   * @birth: get_cycles time when this RPC was added to the grantable
   * list. Invalid if RPC isn't in the grantable list.
   */
  ap_uint<64> birth;
  
  /**
   * @copied_out: All of the bytes of the message with offset less
   * than this value have been copied to user-space buffers.
   */
  int copied_out;
};

/**
 * struct homa_rpc - One of these structures exists for each active
 * RPC. The same structure is used to manage both outgoing RPCs on
 * clients and incoming RPCs on servers.
 */
struct homa_rpc_t {
  /** @output_offset:  Offset in DMA to write result. */
  int output_offset;

  /**
   * @state: The current state of this RPC:
   *
   * @RPC_OUTGOING:     The RPC is waiting for @msgout to be transmitted
   *                    to the peer.
   * @RPC_INCOMING:     The RPC is waiting for data @msgin to be received
   *                    from the peer; at least one packet has already
   *                    been received.
   * @RPC_IN_SERVICE:   Used only for server RPCs: the request message
   *                    has been read from the socket, but the response
   *                    message has not yet been presented to the kernel.
   * @RPC_DEAD:         RPC has been deleted and is waiting to be
   *                    reaped. In some cases, information in the RPC
   *                    structure may be accessed in this state.
   *
   * Client RPCs pass through states in the following order:
   * RPC_OUTGOING, RPC_INCOMING, RPC_DEAD.
   *
   * Server RPCs pass through states in the following order:
   * RPC_INCOMING, RPC_IN_SERVICE, RPC_OUTGOING, RPC_DEAD.
   */
  enum {
    RPC_OUTGOING            = 5,
    RPC_INCOMING            = 6,
    RPC_IN_SERVICE          = 8,
    RPC_DEAD                = 9
  } state;

  /* Valid bits for @flags:
   * RPC_PKTS_READY -        The RPC has input packets ready to be
   *                         copied to user space.
   * RPC_COPYING_FROM_USER - Data is being copied from user space into
   *                         the RPC; the RPC must not be reaped.
   * RPC_COPYING_TO_USER -   Data is being copied from this RPC to
   *                         user space; the RPC must not be reaped.
   * RPC_HANDING_OFF -       This RPC is in the process of being
   *                         handed off to a waiting thread; it must
   *                         not be reaped.
   */
#define RPC_PKTS_READY        1
#define RPC_COPYING_FROM_USER 2
#define RPC_COPYING_TO_USER   4
#define RPC_HANDING_OFF       8
#define RPC_CANT_REAP (RPC_COPYING_FROM_USER | RPC_COPYING_TO_USER | RPC_HANDING_OFF)

  /**
   * @flags: Additional state information: an OR'ed combination of
   * various single-bit flags. See below for definitions. Must be
   * manipulated with atomic operations because some of the manipulations
   * occur without holding the RPC lock.
   */
  int flags;

  /**
   * @grants_in_progress: Count of active grant sends for this RPC;
   * it's not safe to reap the RPC unless this value is zero.
   * This variable is needed so that grantable_lock can be released
   * while sending grants, to reduce contention.
   */
  int grants_in_progress;

  /**
   * @peer: Information about the other machine (the server, if
   * this is a client RPC, or the client, if this is a server RPC).
   */
  homa_peer_id_t peer;
  
  /** @dport: Port number on @peer that will handle packets. */
  //ap_uint<16> dport;
  
  /**
   * @id: Unique identifier for the RPC among all those issued
   * from its port.
   */
  homa_rpc_id_t id;
  
  /**
   * @completion_cookie: Only used on clients. Contains identifying
   * information about the RPC provided by the application; returned to
   * the application with the RPC's result.
   */
  ap_uint<64> completion_cookie;

  /**
   * @msgin: Information about the message we receive for this RPC
   * (for server RPCs this is the request, for client RPCs this is the
   * response).
   */
  homa_message_in_t msgin;
	
  /**
   * @msgout: Information about the message we send for this RPC
   * (for client RPCs this is the request, for server RPCs this is the
   * response).
   */
  homa_message_out_t msgout;

  /**
   * @silent_ticks: Number of times homa_timer has been invoked
   * since the last time a packet indicating progress was received
   * for this RPC, so we don't need to send a resend for a while.
   */
  int silent_ticks;

  /**
   * @resend_timer_ticks: Value of homa->timer_ticks the last time
   * we sent a RESEND for this RPC.
   */
  ap_uint<32> resend_timer_ticks;

  /**
   * @done_timer_ticks: The value of homa->timer_ticks the first
   * time we noticed that this (server) RPC is done (all response
   * packets have been transmitted), so we're ready for an ack.
   * Zero means we haven't reached that point yet.
   */
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
};

homa_rpc_t & homa_rpc_new_client(homa_t * homa, int & output_offset, in6_addr & dest_addr);
homa_rpc_t & homa_rpc_find(ap_uint<64> id);

extern homa_rpc_t rpcs[MAX_RPCS];
extern homa_rpc_stack_t rpc_stack;

#endif
