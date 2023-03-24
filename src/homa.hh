//#define HOMA_IPV6_HEADER_LENGTH 40
//#define HOMA_IPV4_HEADER_LENGTH 40

#define HOMA_MAX_MESSAGE_LENGTH 1000000

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

struct homa_rpc {
		RPC_INCOMING            = 6,
		RPC_IN_SERVICE          = 8,
		RPC_DEAD                = 9
	} state;
	atomic_t flags;
	atomic_t grants_in_progress;
	struct homa_peer *peer;
	__u16 dport;
	__u64 id;
	__u64 completion_cookie;
	int error;
	struct homa_message_in msgin;
	/**
	 * @msgout: Information about the message we send for this RPC
	 * (for client RPCs this is the request, for server RPCs this is the
	 * response).
	 */
	struct homa_message_out msgout;
	/**
	 * @hash_links: Used to link this object into a hash bucket for
	 * either @hsk->client_rpc_buckets (for a client RPC), or
	 * @hsk->server_rpc_buckets (for a server RPC).
	 */
	struct hlist_node hash_links;
	/**
	 * @ready_links: Used to link this object into
	 * &homa_sock.ready_requests or &homa_sock.ready_responses.
	 */
	struct list_head ready_links;
	/**
	 * @active_links: For linking this object into @hsk->active_rpcs.
	 * The next field will be LIST_POISON1 if this RPC hasn't yet been
	 * linked into @hsk->active_rpcs. Access with RCU.
	 */
	struct list_head active_links;
	/** @dead_links: For linking this object into @hsk->dead_rpcs. */
	struct list_head dead_links;
	/**
	 * @interest: Describes a thread that wants to be notified when
	 * msgin is complete, or NULL if none.
	 */
	struct homa_interest *interest;
	/**
	 * @grantable_links: Used to link this RPC into peer->grantable_rpcs.
	 * If this RPC isn't in peer->grantable_rpcs, this is an empty
	 * list pointing to itself.
	 */
	struct list_head grantable_links;
	/**
	 * @throttled_links: Used to link this RPC into homa->throttled_rpcs.
	 * If this RPC isn't in homa->throttled_rpcs, this is an empty
	 * list pointing to itself.
	 */
	struct list_head throttled_links;
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
	__u32 resend_timer_ticks;
	/**
	 * @done_timer_ticks: The value of homa->timer_ticks the first
	 * time we noticed that this (server) RPC is done (all response
	 * packets have been transmitted), so we're ready for an ack.
	 * Zero means we haven't reached that point yet.
	 */
	__u32 done_timer_ticks;
	/**
	 * @magic: when the RPC is alive, this holds a distinct value that
	 * is unlikely to occur naturally. The value is cleared when the
	 * RPC is reaped, so we can detect accidental use of an RPC after
	 * it has been reaped.
	 */
#define HOMA_RPC_MAGIC 0xdeadbeef
	int magic;
	/**
	 * @start_cycles: time (from get_cycles()) when this RPC was created.
	 * Used (sometimes) for testing.
	 */
	uint64_t start_cycles;
};

// TODO use union to pick between packet types?

// Maximum size of ethernet assuming non-jumbo frames:
//     6*8 + 6*8 + 4*8 + 2*8 + 1500*8 + 4*8 = 12176 bits
typedef hls::axis<ap_uint<12176>, 0, 0, 0> raw_frame;

struct eth_header_t {
  ap_uint<48> mac_dest;
  ap_uint<48> mac_src;
  ap_uint<32> tag;
  ap_uint<16> ethertype;
};

template<int option_size>
struct ipv4_header_t {
  ap_uint<4>  version;
  ap_uint<4>  ihl;
  ap_uint<6>  dscp;
  ap_uint<2>  ecn;
  ap_uint<16> length;
  ap_uint<16> id;
  ap_uint<3>  flags;
  ap_uint<13> offset;
  ap_uint<8>  ttl;
  ap_uint<8>  protocol;
  ap_uint<16> checksum;
  ap_uint<32> source;
  ap_uint<32> dest;
  ap_uint<D>  options;
};

template<int ipv4_option_size,
	 int ipv4_payload_size>
struct ipv4_t {
  ipv4_header_t<ipv4_option_size> header;
  ap_uint<ipv4_payload_size>      payload;
};

template<int ipv4_option_size,
	 int ipv4_payload_size,
	 int homa_payload_size>
struct eth_t {
  eth_header_t                                header;
  ipv4_t<ipv4_option_size, ipv4_payload_size> ipv4;
  ap_uint<32>                                 crc;
};

struct rpc_t {
  ap_uint<32> id;
  ap_uint<>
};
