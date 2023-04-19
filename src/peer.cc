#include "peer.hh"
#include "hash.hh"

homa_peer_t peers[HOMA_PEERTAB_BUCKETS];

/**
 * homa_peer_find() - 
 * @addr:       
 * Return:      
 */
//homa_peer_id_t homa_peer_find(in6_addr & addr) {
//homa_peer_id_t homa_peer_find() {
//  // TODO collisions are ignored. Each address takes an entire bucket.
//  //homa_peer_id_t bucket = hash_32(addr.s6_addr[0], HOMA_PEERTAB_BUCKET_BITS);
//  //bucket ^= (homa_peer_id_t) hash_32(addr.s6_addr[1], HOMA_PEERTAB_BUCKET_BITS);
//  //bucket ^= (homa_peer_id_t) hash_32(addr.s6_addr[2], HOMA_PEERTAB_BUCKET_BITS);
//  //bucket ^= (homa_peer_id_t) hash_32(addr.s6_addr[3], HOMA_PEERTAB_BUCKET_BITS);
//
//  //homa_peer_t & peer = peers[bucket];
//
//  // Naive address lookup. Should at least be bounded
//  for (int i = 0; i < HOMA_PEERTAB_BUCKETS; ++i) {
//#pragma HLS unroll
//    if (dest_addr == peers[i]) {
//
//    }
//  }
//
//  // TODO This always genereates a new peer even if there is a collision 
//  peer.addr = addr;
//
//  peer.unsched_cutoffs[HOMA_MAX_PRIORITIES-1] = 0;
//  peer.unsched_cutoffs[HOMA_MAX_PRIORITIES-2] = INT_MAX;
//  peer.cutoff_version = 0;
//  peer.last_update_jiffies = 0;
//  peer.outstanding_resends = 0;
//  peer.most_recent_resend = 0;
//  // peer.least_recent_rpc = NULL; // TODO 
//  peer.least_recent_ticks = 0;
//  peer.current_ticks = -1;
//  // peer.resend_rpc = NULL; // TODO 
//  peer.num_acks = 0;
//
//  return bucket;
//}
//
//// TODO need a cycle bounded homa_peer find, server_rpc find
