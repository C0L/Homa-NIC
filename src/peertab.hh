/**
 * define HOMA_MAX_PRIORITIES - The maximum number of priority levels that
 * Homa can use (the actual number can be restricted to less than this at
 * runtime). Changing this value will affect packet formats.
 */
#define HOMA_MAX_PRIORITIES 8

/**
 * define NUM_PEER_UNACKED_IDS - The number of ids for unacked RPCs that
 * can be stored in a struct homa_peer.
 */
#define NUM_PEER_UNACKED_IDS 5

/**
 * define HOMA_PEERTAB_BUCKETS - Number of bits in the bucket index for a
 * homa_peertab.  Should be large enough to hold an entry for every server
 * in a datacenter without long hash chains.
 */
#define HOMA_PEERTAB_BUCKET_BITS 15

/** define HOME_PEERTAB_BUCKETS - Number of buckets in a homa_peertab. */
#define HOMA_PEERTAB_BUCKETS (1 << HOMA_PEERTAB_BUCKET_BITS)

typedef ap_uint<HOMA_PEERTAB_BUCKET_BITS> homa_peer_id_t;

/**
 * struct homa_ack - Identifies an RPC that can be safely deleted by its
 * server. After sending the response for an RPC, the server must retain its
 * state for the RPC until it knows that the client has successfully
 * received the entire response. An ack indicates this. Clients will
 * piggyback acks on future data packets, but if a client doesn't send
 * any data to the server, the server will eventually request an ack
 * explicitly with a NEED_ACK packet, in which case the client will
 * return an explicit ACK.
 */
struct homa_ack_t {
	/**
	 * @id: The client's identifier for the RPC. 0 means this ack
	 * is invalid.
	 */
	ap_uint<64> client_id;

	/** @client_port: The client-side port for the RPC. */
	ap_uint<16> client_port;

	/** @server_port: The server-side port for the RPC. */
	ap_uint<16> server_port;
}; 

/**
 * struct homa_peer - One of these objects exists for each machine that we
 * have communicated with (either as client or server).
 */
struct homa_peer_t {
  /**
   * @addr: IPv6 address for the machine (IPv4 addresses are stored
   * as IPv4-mapped IPv6 addresses).
   */
  ap_uint<64> addr;

  /**
   * @unsched_cutoffs: priorities to use for unscheduled packets
   * sent to this host, as specified in the most recent CUTOFFS
   * packet from that host. See documentation for @homa.unsched_cutoffs
   * for the meanings of these values.
   */
  int unsched_cutoffs[HOMA_MAX_PRIORITIES];

  /**
   * @cutoff_version: value of cutoff_version in the most recent
   * CUTOFFS packet received from this peer.  0 means we haven't
   * yet received a CUTOFFS packet from the host. Note that this is
   * stored in network byte order.
   */
  ap_uint<16> cutoff_version;

  /**
   * last_update_jiffies: time in jiffies when we sent the most
   * recent CUTOFFS packet to this peer.
   */
  unsigned long last_update_jiffies;
  
  /**
   * grantable_rpcs: Contains all homa_rpcs (both requests and
   * responses) involving this peer whose msgins require (or required
   * them in the past) and have not been fully received. The list is
   * sorted in priority order (head has fewest bytes_remaining).
   * Locked with homa->grantable_lock.
   */
  //struct list_head grantable_rpcs;

  /**
   * @grantable_links: Used to link this peer into homa->grantable_peers,
   * if there are entries in grantable_rpcs. If grantable_rpcs is empty,
   * this is an empty list pointing to itself.
   */
  //struct list_head grantable_links;

  // Colin - we are assuming there are no hash collisions
  /**
   * @peertab_links: Links this object into a bucket of its
   * homa_peertab.
   */
  //struct hlist_node peertab_links;

  /**
   * @outstanding_resends: the number of resend requests we have
   * sent to this server (spaced @homa.resend_interval apart) since
   * we received a packet from this peer.
   */
  int outstanding_resends;

  /**
   * @most_recent_resend: @homa->timer_ticks when the most recent
   * resend was sent to this peer.
   */
  int most_recent_resend;

  /**
   * @least_recent_rpc: of all the RPCs for this peer scanned at
   * @current_ticks, this is the RPC whose @resend_timer_ticks
   * is farthest in the past.
   */
  homa_rpc_id_t least_recent_rpc;

  /**
   * @least_recent_ticks: the @resend_timer_ticks value for
   * @least_recent_rpc.
   */
  ap_uint<32> least_recent_ticks;

  /**
   * @current_ticks: the value of @homa->timer_ticks the last time
   * that @least_recent_rpc and @least_recent_ticks were computed.
   * Used to detect the start of a new homa_timer pass.
   */
  ap_uint<32> current_ticks;

  /**
   * @resend_rpc: the value of @least_recent_rpc computed in the
   * previous homa_timer pass. This RPC will be issued a RESEND
   * in the current pass, if it still needs one.
   */
  homa_rpc_id_t resend_rpc;

  /**
   * @num_acks: the number of (initial) entries in @acks that
   * currently hold valid information.
   */
  int num_acks;

  /**
   * @acks: info about client RPCs whose results have been completely
   * received.
   */
  homa_ack_t acks[NUM_PEER_UNACKED_IDS];
};
	
// TODO 
homa_peer_id_t homa_peer_find(...);
