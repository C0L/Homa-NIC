#include "peer.hh"
#include "hash.hh"

homa_peer_t homa_peer[HOMA_PEERTAB_BUCKETS];

/**
 * homa_peer_find() - Returns the peer associated with a given host; creates
 * a new homa_peer if one doesn't already exist.
 * @addr:       Address of the desired host: IPv4 addresses are represented
 *              as IPv4-mapped IPv6 addresses.
 * Return:      The peer associated with @addr, or a new peer if one if DNE
 */
homa_peer_id_t homa_peer_find(in6_addr & addr) {
  // TODO collisions are ignored. Each address takes an entire bucket.
  homa_peer_id_t bucket = hash_32(addr.s6_addr[0], HOMA_PEERTAB_BUCKET_BITS);
  bucket ^= hash_32(addr.s6_addr[1], HOMA_PEERTAB_BUCKET_BITS);
  bucket ^= hash_32(addr.s6_addr[2], HOMA_PEERTAB_BUCKET_BITS);
  bucket ^= hash_32(addr.s6_addr[3], HOMA_PEERTAB_BUCKET_BITS);

  homa_peer_t & peer = homa_peer[bucket];

  // TODO This always genereates a new peer?!
  
  peer.addr = addr;
  peer.unsched_cutoffs[HOMA_MAX_PRIORITIES-1] = 0;
  peer.unsched_cutoffs[HOMA_MAX_PRIORITIES-2] = INT_MAX;
  peer.cutoff_version = 0;
  peer.last_update_jiffies = 0;
  peer.outstanding_resends = 0;
  peer.most_recent_resend = 0;
  // peer.least_recent_rpc = NULL; // TODO 
  peer.least_recent_ticks = 0;
  peer.current_ticks = -1;
  // peer.resend_rpc = NULL; // TODO 
  peer.num_acks = 0;

  return bucket;
}
