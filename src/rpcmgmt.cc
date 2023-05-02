#include "rpcmgmt.hh"
#include "cam.hh"

void update_rpc_stack(hls::stream<rpc_id_t> & rpc_stack_next,
		      hls::stream<rpc_id_t> & rpc_stack_free) {

  static stack_t<rpc_id_t, MAX_RPCS> rpc_stack(true);

#pragma HLS pipeline II=1

  rpc_id_t freed_rpc;

  // TODO can we do away with null RPC that???
  if (!rpc_stack.empty()) {
    rpc_id_t next_rpc = rpc_stack.pop();
    rpc_stack_next.write(next_rpc+1);
  }

  // TODO expand the rpc_next stream
  if (rpc_stack_free.read_nb(freed_rpc)) {
    rpc_stack.push(freed_rpc-1);
  }
}

// TODO add support for deletion
void update_rpc_table(hls::stream<rpc_hashpack_t> & rpc_table_request,
		      hls::stream<rpc_id_t> & rpc_table_response,
		      hls::stream<homa_rpc_t> & rpc_table_insert) {
  static entry_t<rpc_hashpack_t,rpc_id_t> table_0[RPC_SUB_TABLE_SIZE];
  static entry_t<rpc_hashpack_t,rpc_id_t> table_1[RPC_SUB_TABLE_SIZE];
  static entry_t<rpc_hashpack_t,rpc_id_t> table_2[RPC_SUB_TABLE_SIZE];
  static entry_t<rpc_hashpack_t,rpc_id_t> table_3[RPC_SUB_TABLE_SIZE];

  static cam_t<rpc_hashpack_t,table_op_t<rpc_hashpack_t,rpc_id_t>, MAX_OPS> ops;

#pragma HLS dependence variable=ops inter RAW false
#pragma HLS dependence variable=ops inter WAR false

#pragma HLS dependence variable=table_0 inter RAW false
#pragma HLS dependence variable=table_1 inter RAW false
#pragma HLS dependence variable=table_2 inter RAW false
#pragma HLS dependence variable=table_3 inter RAW false
  
#pragma HLS dependence variable=table_0 inter WAR false
#pragma HLS dependence variable=table_1 inter WAR false
#pragma HLS dependence variable=table_2 inter WAR false
#pragma HLS dependence variable=table_3 inter WAR false

#pragma HLS pipeline II=1

  if (!rpc_table_request.empty()) {
    rpc_hashpack_t query;
    rpc_table_request.read(query);

    rpc_id_t rpc_id;
    table_op_t<rpc_hashpack_t,rpc_id_t> cam_id;

    entry_t<rpc_hashpack_t,rpc_id_t> search_0 = table_0[murmur3_32((uint32_t *) &query, 7, SEED0)];
    entry_t<rpc_hashpack_t,rpc_id_t> search_1 = table_1[murmur3_32((uint32_t *) &query, 7, SEED1)];
    entry_t<rpc_hashpack_t,rpc_id_t> search_2 = table_2[murmur3_32((uint32_t *) &query, 7, SEED2)];
    entry_t<rpc_hashpack_t,rpc_id_t> search_3 = table_3[murmur3_32((uint32_t *) &query, 7, SEED3)];

    if (search_0.hashpack == query) rpc_id = search_0.id;
    if (search_1.hashpack == query) rpc_id = search_1.id;
    if (search_2.hashpack == query) rpc_id = search_2.id;
    if (search_3.hashpack == query) rpc_id = search_3.id;
    if (ops.search(query, cam_id)) rpc_id = cam_id.entry.id;

    rpc_table_response.write(rpc_id);
  } else if (!rpc_table_insert.empty()) {
    homa_rpc_t insertion;
    rpc_table_insert.read(insertion);

    rpc_hashpack_t hashpack = {insertion.addr, insertion.rpc_id, insertion.dport, 0};

    ops.push({0, {hashpack, insertion.rpc_id}});
  } else if (!ops.empty()) {
    entry_t<rpc_hashpack_t,rpc_id_t> out_entry;

    table_op_t<rpc_hashpack_t,rpc_id_t> op = ops.pop();

    if (op.table_id == 0) {
      uint32_t hash = murmur3_32((uint32_t *) &op.entry.hashpack, 7, SEED0);
      out_entry = table_0[hash];
      table_0[hash] = op.entry;
    } else if (op.table_id == 1) {
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
    
    if (!out_entry.id)
      ops.push({op.table_id++, op.entry});
  }
}

void update_rpc_buffer(hls::stream<rpc_id_t> & rpc_buffer_request_primary,
		       hls::stream<homa_rpc_t> & rpc_buffer_response_primary,
		       hls::stream<rpc_id_t> & rpc_buffer_request_secondary,
		       hls::stream<homa_rpc_t> & rpc_buffer_response_secondary,
		       hls::stream<homa_rpc_t> & rpc_buffer_insert) {
  // Actual RPC data
  static homa_rpc_t rpcs[MAX_RPCS];
#pragma HLS pipeline II=1

  rpc_id_t query;

  if (rpc_buffer_request_primary.read_nb(query)) {
    rpc_buffer_response_primary.write(rpcs[query]);
  } else if (rpc_buffer_request_secondary.read_nb(query)) {
    rpc_buffer_response_secondary.write(rpcs[query]);
  }

  homa_rpc_t update;
  if (rpc_buffer_insert.read_nb(update)) {
    rpcs[update.rpc_id] = update;
  } 
}

void update_peer_stack(hls::stream<peer_id_t> & peer_stack_next,
		       hls::stream<peer_id_t> & peer_stack_free) {
  static stack_t<peer_id_t, MAX_RPCS> peer_stack(true);

#pragma HLS pipeline II=1
  // TODO compare to srpt queue?
  if (!peer_stack.empty()) {
    peer_id_t next_peer = peer_stack.pop();
    peer_stack_next.write(next_peer);
  }

  peer_id_t freed_peer;

  if (peer_stack_free.read_nb(freed_peer)) {
    peer_stack.push(freed_peer);
  }

  //peer_id_t freed_peer;
  //if (peer_stack_free.read_nb(freed_peer)) {
  //  peer_stack.push(freed_peer);
  //}

  //peer_id_t new_peer;
  //if (!peer_stack_next.full() && !peer_stack.empty()) {
  //  new_peer = peer_stack.pop();
  //  peer_stack_next.write(new_peer);
  //}
}


// TODO add support for deletion
void update_peer_table(hls::stream<peer_hashpack_t> & peer_table_request,
		       hls::stream<peer_id_t> & peer_table_response,
		       hls::stream<homa_peer_t> & peer_table_insert) {
  static entry_t<peer_hashpack_t,peer_id_t> table_0[RPC_SUB_TABLE_SIZE];
  static entry_t<peer_hashpack_t,peer_id_t> table_1[RPC_SUB_TABLE_SIZE];
  static entry_t<peer_hashpack_t,peer_id_t> table_2[RPC_SUB_TABLE_SIZE];
  static entry_t<peer_hashpack_t,peer_id_t> table_3[RPC_SUB_TABLE_SIZE];

  static cam_t<peer_hashpack_t,table_op_t<peer_hashpack_t,peer_id_t>, MAX_OPS> ops;

#pragma HLS dependence variable=ops inter RAW false
#pragma HLS dependence variable=ops inter WAR false

#pragma HLS dependence variable=table_0 inter RAW false
#pragma HLS dependence variable=table_1 inter RAW false
#pragma HLS dependence variable=table_2 inter RAW false
#pragma HLS dependence variable=table_3 inter RAW false
  
#pragma HLS dependence variable=table_0 inter WAR false
#pragma HLS dependence variable=table_1 inter WAR false
#pragma HLS dependence variable=table_2 inter WAR false
#pragma HLS dependence variable=table_3 inter WAR false

#pragma HLS pipeline II=1

  if (!peer_table_request.empty()) {
    peer_hashpack_t query;
    peer_table_request.read(query);

    peer_id_t peer_id = 0;
    table_op_t<peer_hashpack_t,peer_id_t> cam_id;

    entry_t<peer_hashpack_t,peer_id_t> search_0 = table_0[murmur3_32((uint32_t *) &query, 7, SEED0)];
    entry_t<peer_hashpack_t,peer_id_t> search_1 = table_1[murmur3_32((uint32_t *) &query, 7, SEED1)];
    entry_t<peer_hashpack_t,peer_id_t> search_2 = table_2[murmur3_32((uint32_t *) &query, 7, SEED2)];
    entry_t<peer_hashpack_t,peer_id_t> search_3 = table_3[murmur3_32((uint32_t *) &query, 7, SEED3)];

    if (search_0.hashpack == query) peer_id = search_0.id;
    if (search_1.hashpack == query) peer_id = search_1.id;
    if (search_2.hashpack == query) peer_id = search_2.id;
    if (search_3.hashpack == query) peer_id = search_3.id;
    if (ops.search(query, cam_id))  peer_id = cam_id.entry.id;

    // If peer id was not updated, then this will return 0, indicating no hit was found
    peer_table_response.write(peer_id);
  } else if (!peer_table_insert.empty()) {
    homa_peer_t insertion;
    peer_table_insert.read(insertion);

    peer_hashpack_t hashpack = {insertion.addr};

    ops.push({0, {hashpack, insertion.peer_id}});
  } else if (!ops.empty()) {
    entry_t<peer_hashpack_t,peer_id_t> out_entry;

    table_op_t<peer_hashpack_t,peer_id_t> op = ops.pop();

    if (op.table_id == 0) {
      uint32_t hash = murmur3_32((uint32_t *) &op.entry.hashpack, 7, SEED0);
      out_entry = table_0[hash];
      table_0[hash] = op.entry;
    } else if (op.table_id == 1) {
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
    
    if (!out_entry.id)
      ops.push({op.table_id++, op.entry});
  }
}

void update_peer_buffer(hls::stream<peer_id_t> & peer_buffer_request_primary,
			hls::stream<homa_peer_t> & peer_buffer_response_primary,
			hls::stream<homa_peer_t> & peer_buffer_insert) {
  // Actual RPC data
  static homa_peer_t peers[MAX_PEERS];
#pragma HLS pipeline II=1

  peer_id_t query;

  if (peer_buffer_request_primary.read_nb(query)) {
    peer_buffer_response_primary.write(peers[query]);
  } 

  homa_peer_t update;
  if (peer_buffer_insert.read_nb(update)) {
    peers[update.peer_id] = update;
  } 
}

uint32_t murmur_32_scramble(uint32_t k) {
#pragma HLS pipeline II=1
  k *= 0xcc9e2d51;
  k = (k << 15) | (k >> 17);
  k *= 0x1b873593;
  return k;
}

uint32_t murmur3_32(const uint32_t * key, int len, uint32_t seed) {
#pragma HLS pipeline II=1
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
