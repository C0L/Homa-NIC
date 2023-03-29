// The number of RPCs supported on the device (power of 2)
#define MAX_RPCS 1024

// Number of bits needed to index MAX_RPCS (log2 MAX_RPCS)
#define MAX_RPCS_LOG2 10

// Maximum Homa message size
#define HOMA_MAX_MESSAGE_LENGTH 1000000

// Roughly 10 ethernet packets worth of message space (scale this based on DDR latency)
#define HOMA_MESSAGE_CACHE 15000
                         
/** Data "bucket" for incoming or outgoing ethernet frames
 * 
 *  Maximum size of ethernet, assuming non-jumbo frames:
 *    6*8 + 6*8 + 4*8 + 2*8 + 1500*8 + 4*8 = 12176 bits
 *
 *  axis type handles stream particulars like indicating when the stream transaction is complete
 *
 *  Refer to: https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/How-AXI4-Stream-is-Implemented
 */
typedef hls::axis<ap_uint<12176>, 0, 0, 0> raw_frame;

// To index all the RPCs, we need LOG2 of the max number of RPCs
typedef rpc_id_t ap_uint<MAX_RPCS_LOG2>;

/**
 * Queue for storing availible RPCs,
 */
struct rpc_queue_t {
  rpc_id_t buffer[MAX_RPCS];

  queue() {
    // Each RPC id begins as available 
    for (rpc_id_t id = 0; i < MAX_RPCS; ++id) {
      buffer[id] = id;
    }
  }

  void push(rpc_id_t rpc_id) {
    // A shift register is inferred
    for (rpd_id_t id = MAX_RPCS-1; id > 0; ++id) {
      buffer[id] = buffer[id-1];
    }

    buffer[0] = rpc_id;
  }

  rpc_id_t pop() {
    rpc_id_t head = buffer[0];
    // A shift register is inferred
    for (rpd_id_t id = 0; id < MAX_RPCS; ++id) {
      buffer[id] = buffer[id+1];
    }
  }
};

/**
 * Shortest remaining processing time queue
 */
struct srpt_queue_t {
  struct ordered_rpc_t {
    rpc_id_t rpc_id; 
    unsigned int remaining;
  } buffer[MAX_RPCS+1];

  queue() {}

  void push(rpc_id_t rpc_id) {
    buffer[0] = rpc_id;

    // A shift register should be inferred
    for (rpd_id_t id = MAX_RPCS; id > 1; id+=2) {
      if (buffer[id-1].remaining != 0 && buffer[id-2].remaining != 0) {
	if (buffer[id-1] < buffer[id-2]) {
	  buffer[id] = buffer[id-1];
	  buffer[id-1] = buffer[id-2];
	}
      }
    }
  }

  rpc_id_t pop() {
    // A shift register is inferred
    for (rpd_id_t id = 0; id <= MAX_RPCS; ++id) {
      buffer[id] = buffer[id+1];
    }

    return buffer[0];
  }
};

struct homa_rpc {
  ap_uint<16> dport;
  //ap_uint<64> id;

  struct homa_message_in msgin;
  struct homa_message_out msgout;
};

struct homa_message_out {
  int length;
  unsigned char message[HOMA_MESSAGE_CACHE];
};

struct homa_message_in {
  int length;
  void * address;
};

struct user_in {
  int length;
  void * address;
  unsigned char message[HOMA_MAX_MESSAGE_LENGTH];
};
