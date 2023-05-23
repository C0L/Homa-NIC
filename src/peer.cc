#include "peer.hh"
#include "stack.hh"
#include "hashmap.hh"

// TODO II = 2??
void update_peer_stack(hls::stream<peer_id_t> & peer_stack_next_primary,
		       hls::stream<peer_id_t> & peer_stack_next_secondary,
		       hls::stream<peer_id_t> & peer_stack_free) {

  static stack_t<peer_id_t, MAX_PEERS> peer_stack(true);

#pragma HLS pipeline II=1

  if (!peer_stack.empty()) {
    if (!peer_stack_next_primary.full()) {
      peer_id_t next_peer = peer_stack.pop();
      peer_stack_next_primary.write(next_peer+1);
    } else if(!peer_stack_next_secondary.full()) {
      peer_id_t next_peer = peer_stack.pop();
      peer_stack_next_secondary.write(next_peer+1);
    }
  }

  peer_id_t freed_peer;
  if (peer_stack_free.read_nb(freed_peer)) {
    peer_stack.push(freed_peer-1);
  }
}


// TODO add support for deletion
void update_peer_table(hls::stream<peer_hashpack_t> & peer_table_request_primary,
		       hls::stream<peer_id_t> & peer_table_response_primary,
		       hls::stream<peer_hashpack_t> & peer_table_request_secondary,
		       hls::stream<peer_id_t> & peer_table_response_secondary,
		       hls::stream<homa_peer_t> & peer_table_insert_primary,
		       hls::stream<homa_peer_t> & peer_table_insert_secondary) {

  static hashmap_t<peer_hashpack_t, peer_id_t, PEER_SUB_TABLE_SIZE, PEER_SUB_TABLE_INDEX, PEER_HP_SIZE, MAX_OPS> hashmap;
  // TODO check these deps
#pragma HLS dependence variable=hashmap inter WAR false
#pragma HLS dependence variable=hashmap inter RAW false

#pragma HLS pipeline II=1

  if (!peer_table_request_primary.empty()) {
    peer_hashpack_t query;
    peer_table_request_primary.read(query);

    peer_id_t peer_id = hashmap.search(query);

    peer_table_response_primary.write(peer_id);
  } else if (!peer_table_request_secondary.empty()) {
    peer_hashpack_t query;
    peer_table_request_secondary.read(query);

    peer_id_t peer_id = hashmap.search(query);

    peer_table_response_secondary.write(peer_id);
  } else if (!peer_table_insert_primary.empty()) {
    homa_peer_t insertion;
    peer_table_insert_primary.read(insertion);

    peer_hashpack_t hashpack = {insertion.addr};

    entry_t<peer_hashpack_t, peer_id_t> entry = {hashpack, insertion.peer_id};
    hashmap.queue(entry);
  } else if (!peer_table_insert_secondary.empty()) {
    homa_peer_t insertion;
    peer_table_insert_secondary.read(insertion);

    peer_hashpack_t hashpack = {insertion.addr};

    entry_t<peer_hashpack_t, peer_id_t> entry = {hashpack, insertion.peer_id};
    hashmap.queue(entry);
  } else {
    hashmap.process();
  }
}

void update_peer_buffer(hls::stream<peer_id_t> & peer_buffer_request_primary,
			hls::stream<homa_peer_t> & peer_buffer_response_primary,
			hls::stream<homa_peer_t> & peer_buffer_insert_primary,
			hls::stream<homa_peer_t> & peer_buffer_insert_secondary) {
  // Actual RPC data
  static homa_peer_t peers[MAX_PEERS];
#pragma HLS pipeline II=1

  peer_id_t query;

  if (peer_buffer_request_primary.read_nb(query)) {
    peer_buffer_response_primary.write(peers[query-1]);
  } 

  homa_peer_t update;
  if (peer_buffer_insert_primary.read_nb(update)) {
    peers[update.peer_id-1] = update;
  } else if (peer_buffer_insert_secondary.read_nb(update)) {
    peers[update.peer_id-1] = update;
  }
}
