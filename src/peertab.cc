static homa_peer_t homa_peer[HOMA_PEERTAB_BUCKETS];

/**
 * homa_peer_find() - Returns the peer associated with a given host; creates
 * a new homa_peer if one doesn't already exist.
 * @addr:       Address of the desired host: IPv4 addresses are represented
 *              as IPv4-mapped IPv6 addresses.
 * @inet:       Socket that will be used for sending packets.
 *
 * Return:      The peer associated with @addr, or a new peer if one if DNE
 */
homa_peer_id_t homa_peer_find(in6_addr & addr, inet_sock & inet) {
  homa_peer_id_t peer;

  // TODO collisions are ignored. Each address takes an entire bucket.
  homa_peer_id_t bucket = hash_32(addr->in6_u.u6_addr32[0], HOMA_PEERTAB_BUCKET_BITS);
  bucket ^= hash_32(addr->in6_u.u6_addr32[1], HOMA_PEERTAB_BUCKET_BITS);
  bucket ^= hash_32(addr->in6_u.u6_addr32[2], HOMA_PEERTAB_BUCKET_BITS);
  bucket ^= hash_32(addr->in6_u.u6_addr32[3], HOMA_PEERTAB_BUCKET_BITS);

  peer = bucket;

  peer.addr = addr;
  peer.unsched_cutoffs[HOMA_MAX_PRIORITIES-1] = 0;
  peer.unsched_cutoffs[HOMA_MAX_PRIORITIES-2] = INT_MAX;
  peer.cutoff_version = 0;
  peer.last_update_jiffies = 0;
  peer.outstanding_resends = 0;
  peer.most_recent_resend = 0;
  peer.least_recent_rpc = NULL;
  peer.least_recent_ticks = 0;
  peer.current_ticks = -1;
  peer.resend_rpc = NULL;
  peer.num_acks = 0;

  return peer;
}
