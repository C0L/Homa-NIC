#include "rpcmgmt.hh"
#include "peer.hh"

void update_rpc_stack(hls::stream<homa_rpc_id_t> rpc_stack_next,
		      hls::stream<homa_rpc_id_t> rpc_stack_free) {
  static homa_rpc_stack_t rpc_stack;

  homa_rpc_id_t freed_rpc;
  if (rpc_stack_free.read_nb(freed_rpc)) {
    rpc_stack_free.push(freed_rpc);
  }

  homa_rpc_id_t new_rpc;
  if (!rpc_stack_next.full() && !rpc_stack.empty()) {
    new_rpc = rpc_stack.pop();
    rpc_stack_next.write(new_rpc);
  }
}

// TODO add support for deletion
void update_rpc_table(hls::stream<hashpack_t> rpc_table_request,
		      hls::stream<homa_rpc_id_t> rpc_table_response,
		      hls::stream<homa_rpc_t> rpc_table_insert) {
  // TODO pipeline?
  static homa_rpc_entry_t table_0[RPC_SUB_TABLE_SIZE];
  static homa_rpc_entry_t table_1[RPC_SUB_TABLE_SIZE];
  static homa_rpc_entry_t table_2[RPC_SUB_TABLE_SIZE];
  static homa_rpc_entry_t table_3[RPC_SUB_TABLE_SIZE];

  static rpc_table_op_t insert_stack[MAX_OPERATIONS];
  static int stack_head = 0;

  // TODO theoretically we can process an insertion and lookup every cycle??? If not then prioritize search
  
  homa_rpc_t insertion;
  // Are there RPCs that we need to insert into the table?
  if (rpc_table_insert.read_nb(insertion)) {
    hashpack_t hashpack = {insertion.dest_addr.sin6_addr.s6_addr, insertion.id, insertion.dport, 0};
    uint32_t hash = murmur3_32((uint32_t *) &hashpack, 7, SEED0);

    homa_rpc_entry_t out_entry = table_0[hash];
    table_0[hash] = insertion;

    // Was there an element at this slot?
    if (out_entry.homa_rpc != 0) {
      insert_stack[stack_head] = {1, out_entry};
      stack_head++;
    }
  } else if (stack_head != 0) {
    stack_head--;
    rpc_table_op_t op = insert_stack[stack_head];
    homa_rpc_entry_t out_entry;

    if (op.table_id == 1) {
      uint32_t hash = murmur3_32((uint32_t *) &op.entry.hashpack, 7, SEED1);
      out_entry = table_1[hash];
      table_1[hash] = op.entry;
    } else if (op.table_id == 2) {
      uint32_t hash = murmur3_32((uint32_t *) &op.entry.hashpack, 7, SEED2);
      out_entry = table_2[hash];
      table_2[hash] = op.entry;
    } else if (op.table_id == 3) {
      uint32_t hash = murmur3_32((uint32_t *) &op.entry.hashpack, 7, SEED3);
      out_entry = table_3[hash];
      table_3[hash] = op.entry;
    }

    // TODO this will ignore infinite loops
    if (out_entry.homa_rpc != 0) {
      insert_stack[stack_head] = {op.entry.hashpack++, out_entry};
      stack_head++;
    }
  }

  hashpack_t query;
  // Are there RPC searches that we need to perform?
  if (rpc_table_request.read_nb(query)) {
    for (int i = 0; i < MAX_OPERATIONS; ++i) {
#pragma unroll
      if (insert_stack[i].entry.hashpack == query) {
	rpc_table_response.write(insert_stack[i].entry.homa_rpc);
	return;
      }
    }

    uint32_t hash_0 = murmur3_32((uint32_t *) &query, 7, SEED0);
    uint32_t hash_1 = murmur3_32((uint32_t *) &query, 7, SEED1);
    uint32_t hash_2 = murmur3_32((uint32_t *) &query, 7, SEED2);
    uint32_t hash_3 = murmur3_32((uint32_t *) &query, 7, SEED3);

    homa_rpc_entry_t search_0 = table_0[hash_0];
    homa_rpc_entry_t search_1 = table_1[hash_1];
    homa_rpc_entry_t search_2 = table_2[hash_2];
    homa_rpc_entry_t search_3 = table_3[hash_3];

    homa_rpc_id_t result;
    
    if (search_0.hashpack == query) result = search_0.homa_rpc;
    if (search_1.hashpack == query) result = search_1.homa_rpc;
    if (search_2.hashpack == query) result = search_2.homa_rpc;
    if (search_3.hashpack == query) result = search_3.homa_rpc;

    rpc_table_response.write(result);
  }
}

void update_rpc_buffer(hls::stream<homa_rpc_id_t> rpc_buffer_request,
		       hls::stream<homa_rpc_t> rpc_buffer_response,
		       hls::stream<homa_rpc_t> rpc_buffer_insert) {
  // Actual RPC data
  static homa_rpc_t rpcs[MAX_RPCS];

  homa_rpc_id_t query;

  if (rpc_buffer_request.read_nb(query)) {
    rpc_buffer_response.write(rpcs[query]);
  }

  homa_rpc_t update;
  if (rpc_buffer_insert.read_nb(update)) {
    rpcs[update.local_id] = update;
  } 
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






















//void rpc_server_insert(hashpack_t & hashpack, homa_rpc_id_t & homa_rpc_id) {
//  homa_rpc_entry_t in_entry = {hashpack, homa_rpc_id};
//  homa_rpc_entry_t out_entry;
//  uint32_t hash;
//
//  for (int iters = 0; iters < 1; ++iters) {
//    hash = murmur3_32((uint32_t *) &in_entry.hashpack, 7, SEED0);
//    out_entry = table_0[hash];
//    table_0[hash] = in_entry;
//
//    // Is there an element at this slot?
//    if (out_entry.homa_rpc == 0) {
//      return;
//    }
//
//    in_entry = out_entry;
//
//    hash = murmur3_32((uint32_t *) &in_entry.hashpack, 7, SEED1);
//    out_entry = table_1[hash];
//    table_1[hash] = in_entry;
//
//    if (out_entry.homa_rpc == 0) {
//      return;
//    }
//
//    in_entry = out_entry;
//
//    hash = murmur3_32((uint32_t *) &in_entry.hashpack, 7, SEED2);
//    out_entry = table_1[hash];
//    table_2[hash] = in_entry;
//
//    if (out_entry.homa_rpc == 0) {
//      return;
//    }
//
//    in_entry = out_entry;
//
//    hash = murmur3_32((uint32_t *) &in_entry.hashpack, 7, SEED3);
//    out_entry = table_3[hash];
//    table_3[hash] = in_entry;
//
//    if (out_entry.homa_rpc == 0) {
//      return;
//    }
//
//    in_entry = out_entry;
//  }
//}
//
//homa_rpc_id_t rpc_server_search(hashpack_t & hashpack) {
//  uint32_t hash_0 = murmur3_32((uint32_t *) &hashpack, 7, SEED0);
//  uint32_t hash_1 = murmur3_32((uint32_t *) &hashpack, 7, SEED1);
//  uint32_t hash_2 = murmur3_32((uint32_t *) &hashpack, 7, SEED2);
//  uint32_t hash_3 = murmur3_32((uint32_t *) &hashpack, 7, SEED3);
//
//  homa_rpc_entry_t & search_0 = table_0[hash_0];
//  homa_rpc_entry_t & search_1 = table_1[hash_1];
//  homa_rpc_entry_t & search_2 = table_2[hash_2];
//  homa_rpc_entry_t & search_3 = table_3[hash_3];
//
//  // TODO clean this up
//  if (search_0.hashpack.s6_addr == hashpack.s6_addr &&
//      search_0.hashpack.port == hashpack.port &&
//      search_0.hashpack.id== hashpack.id) return search_0.homa_rpc;
//  if (search_1.hashpack.s6_addr == hashpack.s6_addr &&
//      search_1.hashpack.port == hashpack.port &&
//      search_1.hashpack.id== hashpack.id) return search_1.homa_rpc;
//  if (search_2.hashpack.s6_addr == hashpack.s6_addr &&
//      search_2.hashpack.port == hashpack.port &&
//      search_2.hashpack.id== hashpack.id) return search_2.homa_rpc;
//  if (search_3.hashpack.s6_addr == hashpack.s6_addr &&
//      search_3.hashpack.port == hashpack.port &&
//      search_3.hashpack.id== hashpack.id) return search_3.homa_rpc;
//  // TODO There is no error handling here!!!
//  // This needs to be removed -- it is just for temporary compilation sake
//  return search_0.homa_rpc;
//}

