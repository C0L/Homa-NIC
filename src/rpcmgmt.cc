#include "rpcmgmt.hh"
#include "peer.hh"

// Binary content addressable memory
// https://github.com/Xilinx/HLS_packet_processing

homa_rpc_stack_t rpc_stack;

homa_rpc_entry_t table_0[RPC_SUB_TABLE_SIZE];
homa_rpc_entry_t table_1[RPC_SUB_TABLE_SIZE];
homa_rpc_entry_t table_2[RPC_SUB_TABLE_SIZE];
homa_rpc_entry_t table_3[RPC_SUB_TABLE_SIZE];

// Actual RPC data
homa_rpc_t rpcs[MAX_RPCS];

uint32_t seeds[4] = {0x7BF6BF21, 0x9FA91FE9, 0xD0C8FBDF, 0xE6D0851C};

homa_rpc_id_t homa_find_server_rpc(sockaddr_in6_t & addr, uint64_t & id) {
  // TODO can remove this
#pragma HLS inline OFF

  hashpack_t hashpack = {addr.sin6_addr.s6_addr, id, (uint32_t) addr.sin6_port};

  return rpc_server_search(hashpack);
}

void homa_insert_server_rpc(sockaddr_in6_t & addr, uint64_t & id, homa_rpc_id_t & homa_rpc_id) {
  // TODO can remove this
#pragma HLS inline OFF

  hashpack_t hashpack = {addr.sin6_addr.s6_addr, id, (uint32_t) addr.sin6_port};

  return rpc_server_insert(hashpack, homa_rpc_id);
}

void rpc_server_insert(hashpack_t & hashpack, homa_rpc_id_t & homa_rpc_id) {
  // TODO can remove this
#pragma HLS inline OFF

  //homa_rpc_id_t insert = homa_rpc_id_t;
  //hashpack_t hashpack = pack;
  homa_rpc_entry_t in_entry = {hashpack, homa_rpc_id};
  homa_rpc_entry_t out_entry;
  uint32_t hash;

  for (int iters = 0; iters < 1; ++iters) {
    hash = murmur3_32((uint32_t *) &in_entry.hashpack, 7, seeds[0]);
    out_entry = table_0[hash];
    table_0[hash] = in_entry;

    // Is there an element at this slot?
    if (out_entry.homa_rpc == 0) {
      return;
    }

    in_entry = out_entry;

    hash = murmur3_32((uint32_t *) &in_entry.hashpack, 7, seeds[1]);
    out_entry = table_1[hash];
    table_1[hash] = in_entry;

    if (out_entry.homa_rpc == 0) {
      return;
    }

    in_entry = out_entry;

    hash = murmur3_32((uint32_t *) &in_entry.hashpack, 7, seeds[2]);
    out_entry = table_1[hash];
    table_2[hash] = in_entry;

    if (out_entry.homa_rpc == 0) {
      return;
    }

    in_entry = out_entry;

    hash = murmur3_32((uint32_t *) &in_entry.hashpack, 7, seeds[3]);
    out_entry = table_3[hash];
    table_3[hash] = in_entry;

    if (out_entry.homa_rpc == 0) {
      return;
    }

    in_entry = out_entry;
  }
}

homa_rpc_id_t rpc_server_search(hashpack_t & hashpack) {
  // TODO can remove this
#pragma HLS inline OFF
  uint32_t hash_0 = murmur3_32((uint32_t *) &hashpack, 7, 0x7BF6BF21);
  uint32_t hash_1 = murmur3_32((uint32_t *) &hashpack, 7, 0x9FA91FE9);
  uint32_t hash_2 = murmur3_32((uint32_t *) &hashpack, 7, 0xD0C8FBDF);
  uint32_t hash_3 = murmur3_32((uint32_t *) &hashpack, 7, 0xE6D0851C);

  homa_rpc_entry_t & search_0 = table_0[hash_0];
  homa_rpc_entry_t & search_1 = table_1[hash_1];
  homa_rpc_entry_t & search_2 = table_2[hash_2];
  homa_rpc_entry_t & search_3 = table_3[hash_3];

  // TODO clean this up
  if (search_0.hashpack.s6_addr == hashpack.s6_addr &&
      search_0.hashpack.port == hashpack.port &&
      search_0.hashpack.id== hashpack.id) return search_0.homa_rpc;
  if (search_1.hashpack.s6_addr == hashpack.s6_addr &&
      search_1.hashpack.port == hashpack.port &&
      search_1.hashpack.id== hashpack.id) return search_1.homa_rpc;
  if (search_2.hashpack.s6_addr == hashpack.s6_addr &&
      search_2.hashpack.port == hashpack.port &&
      search_2.hashpack.id== hashpack.id) return search_2.homa_rpc;
  if (search_3.hashpack.s6_addr == hashpack.s6_addr &&
      search_3.hashpack.port == hashpack.port &&
      search_3.hashpack.id== hashpack.id) return search_3.homa_rpc;

  // TODO There is no error handling here!!!
  // This needs to be removed -- it is just for temporary compilation sake
  return search_0.homa_rpc;
}

uint32_t murmur_32_scramble(uint32_t k) {
  k *= 0xcc9e2d51;
  k = (k << 15) | (k >> 17);
  k *= 0x1b873593;
  return k;
}

uint32_t murmur3_32(const uint32_t * key, int len, uint32_t seed) {
  uint32_t h = seed;
  uint32_t k;

  for (int i = len; i; i--) {
    #pragma HLS unroll
    k = *key;
    key += 1;
    h ^= murmur_32_scramble(k);
    h = (h << 13) | (h >> 19);
    h = h * 5 + 0xe6546b64;
  }

  h ^= murmur_32_scramble(k);

  /* Finalize. */
  h ^= len;
  h ^= h >> 16;
  h *= 0x85ebca6b;
  h ^= h >> 13;
  h *= 0xc2b2ae35;
  h ^= h >> 16;
  return h;
}


