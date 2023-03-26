#define MAX_RPCS 1024
#define MAX_RPCS_LOG2 10
#define HOMA_MAX_MESSAGE_LENGTH 1000000

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

// Used to specify queue initialization strategy
enum QUEUE_INIT {
  ENUMERATE,
  ZERO
};

/**
 * Circular queue for:
 *   1) Storing availible RPCs,
 *   2) Storing messages ready to be transfered to user
 * 
 *  This assumes that log2items = log_2(items) to use interger overflow
 *    to wrap around the queue.
 *
 *   The queue can be initialized with starting values of
 *     either 0...items or all zero.
 */
template<int items, int log2items>
struct queue {
  ap_uint<log2items> circular_buffer[items];
  ap_uint<log2items> head;
  ap_uint<log2items> tail;

  queue(QUEUE_INIT init_type) {
    if (init_type == ENUMERATE) {
      // Each RPC id begins as available 
      for (ap_uint<log2items> i = 0; i < items; ++i)  {
	circular_buffer[i] = i;
      }

      head = 0;
      tail = items;
    }
  }

  void push(ap_uint<log2items> rpc_id) {
    tail++;
    circular_buffer[tail] = rpc_id;
  }

  ap_uint<log2items> pop() {
    ap_uint<log2items> head = circular_buffer[head];
    head++;
    return head;
  }
};


//enum homa_packet_type {
//	DATA               = 0x10,
//	GRANT              = 0x11,
//	RESEND             = 0x12,
//	UNKNOWN            = 0x13,
//	BUSY               = 0x14,
//	CUTOFFS            = 0x15,
//	FREEZE             = 0x16,
//	NEED_ACK           = 0x17,
//	ACK                = 0x18,
//};

//struct common_header {
//	__be16 sport;
//	__be16 dport;
//	__be32 unused1;
//	__be32 unused2;
//	__u8 doff;
//	__u8 type;
//	__u16 unused3;
//	__be16 checksum;
//	__u16 unused4;
//	__be64 sender_id;
//};
//
//
//struct homa_ack {
//	__be64 client_id;
//	__be16 client_port;
//	__be16 server_port;
//};
//
//struct data_segment {
//	__be32 offset;
//	__be32 segment_length;
//	struct homa_ack ack;
//	/** @data: the payload of this segment. */
//	char data[0];
//};
//
//struct data_header {
//	struct common_header common;
//	__be32 message_length;
//	__be32 incoming;
//	__be16 cutoff_version;
//	__u8 retransmit;
//	__u8 pad;
//	struct data_segment seg;
//}
//
//struct grant_header {
//	struct common_header common;
//	__be32 offset;
//	__u8 priority;
//}
//
//struct resend_header {
//	struct common_header common;
//	__be32 offset;
//	__be32 length;
//	__u8 priority;
//};
//
//struct unknown_header {
//	struct common_header common;
//};
//
//struct busy_header {
//	struct common_header common;
//};
//
//struct cutoffs_header {
//	struct common_header common;
//	__be32 unsched_cutoffs[HOMA_MAX_PRIORITIES];
//	__be16 cutoff_version;
//};
//
//struct need_ack_header {
//	/** @common: Fields common to all packet types. */
//	struct common_header common;
//};
//
//struct ack_header {
//	struct common_header common;
//	__be16 num_acks;
//	struct homa_ack acks[NUM_PEER_UNACKED_IDS];
//};

//struct eth_header_t {
//  ap_uint<48> mac_dest;
//  ap_uint<48> mac_src;
//  ap_uint<32> tag;
//  ap_uint<16> ethertype;
//};
//
//template<int option_size>
//struct ipv4_header_t {
//  ap_uint<4>  version;
//  ap_uint<4>  ihl;
//  ap_uint<6>  dscp;
//  ap_uint<2>  ecn;
//  ap_uint<16> length;
//  ap_uint<16> id;
//  ap_uint<3>  flags;
//  ap_uint<13> offset;
//  ap_uint<8>  ttl;
//  ap_uint<8>  protocol;
//  ap_uint<16> checksum;
//  ap_uint<32> source;
//  ap_uint<32> dest;
//  ap_uint<D>  options;
//};
//
//template<int ipv4_option_size,
//	 int ipv4_payload_size>
//struct ipv4_t {
//  ipv4_header_t<ipv4_option_size> header;
//  ap_uint<ipv4_payload_size>      payload;
//};
//
//template<int ipv4_option_size,
//	 int ipv4_payload_size,
//	 int homa_payload_size>
//struct eth_t {
//  eth_header_t                                header;
//  ipv4_t<ipv4_option_size, ipv4_payload_size> ipv4;
//  ap_uint<32>                                 crc;
//};

