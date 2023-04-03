// The number of RPCs supported on the device (power of 2)
#define MAX_RPCS 1024

// Number of bits needed to index MAX_RPCS (log2 MAX_RPCS)
#define MAX_RPCS_LOG2 10

// Maximum Homa message size
#define HOMA_MAX_MESSAGE_LENGTH 1000000

// Roughly 10 ethernet packets worth of message space (scale this based on DDR latency)
#define HOMA_MESSAGE_CACHE 15000

#define HOMA_MAX_HEADER 90

#define ETHERNET_MAX_PAYLOAD 1500
                         
/** Data "bucket" for incoming or outgoing ethernet frames
 * 
 *  Maximum size of ethernet, assuming non-jumbo frames:
 *    6*8 + 6*8 + 4*8 + 2*8 + 1500*8 + 4*8 = 12176 bits
 *
 *  axis type handles stream particulars like indicating when the stream transaction is complete
 *
 *  Refer to: https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/How-AXI4-Stream-is-Implemented
 */
typedef hls::axis<ap_uint<12176>, 0, 0, 0> raw_frame_t;

// To index all the RPCs, we need LOG2 of the max number of RPCs
typedef ap_uint<MAX_RPCS_LOG2> rpc_id_t;

/**
 * Stack for storing availible RPCs,
 */
struct rpc_stack_t {
  rpc_id_t buffer[MAX_RPCS];
  int size;

  rpc_stack_t() {
    // Each RPC id begins as available 
    for (int id = 0; id < MAX_RPCS; ++id) {
      buffer[id] = id;
    }
    size = MAX_RPCS;
  }

  void push(rpc_id_t rpc_id) {
    // A shift register is inferred
    for (int id = MAX_RPCS-1; id > 0; --id) {
      buffer[id] = buffer[id-1];
    }

    buffer[0] = rpc_id;
    size++;
  }

  rpc_id_t pop() {
    rpc_id_t head = buffer[0];
    // A shift register is inferred
    for (int id = 0; id < MAX_RPCS; ++id) {
      buffer[id] = buffer[id+1];
    }

    size--;
    return head;
  }

  int get_size() {
    return size;
  }
};

/**
 * Shortest remaining processing time queue
 */
struct srpt_queue_t {
  struct ordered_rpc_t {
    rpc_id_t rpc_id; 
    unsigned int remaining;
  } buffer[MAX_RPCS+2];

  int size;

  srpt_queue_t() {

    for (int id = 0; id <= MAX_RPCS+1; ++id) {
      buffer[id] = {-1, (unsigned int) -1};
    }

    size = 0;
  }

  void push(rpc_id_t rpc_id, unsigned int remaining) {
    size++;
    buffer[0].rpc_id = rpc_id;
    buffer[0].remaining = remaining;

    for (int id = MAX_RPCS; id > 0; id-=2) {
      if (buffer[id-1].remaining < buffer[id-2].remaining) {
	buffer[id] = buffer[id-2];
      } else {
	buffer[id] = buffer[id-1];
	buffer[id-1] = buffer[id-2];
      }
    }
  }

  rpc_id_t pop() {
    size--;

    for (int id = 0; id <= MAX_RPCS; id+=2) {
      if (buffer[id+1].remaining > buffer[id+2].remaining) {
	buffer[id] = buffer[id+2];
      } else {
	buffer[id] = buffer[id+1];
	buffer[id+1] = buffer[id+2];
      }
    }

    return buffer[0].rpc_id;
  }

  int get_size() {
    return size;
  }
};

struct homa_message_out {
  int length;
  unsigned char message[HOMA_MESSAGE_CACHE];
};

struct homa_message_in {
  int length;
  void * address;
};

struct homa_rpc {
  ap_uint<16> dport;
  //ap_uint<64> id;

  struct homa_message_in msgin;
  struct homa_message_out msgout;
};

struct user_input {
  int output_slot;
  int length;
  unsigned char message[HOMA_MAX_MESSAGE_LENGTH];
};

struct user_output {
  int rpc_id;
  unsigned char message[HOMA_MAX_MESSAGE_LENGTH];
};
