#include "peer.hh"
#include "stack.hh"
#include "hashmap.hh"

void peer_map(stream_t<new_rpc_t, new_rpc_t> & new_rpc_s) {
  static stack_t<peer_id_t, MAX_PEERS> peer_stack(true);
  static hashmap_t<peer_hashpack_t, peer_id_t, PEER_SUB_TABLE_SIZE, PEER_SUB_TABLE_INDEX, PEER_HP_SIZE, MAX_OPS> hashmap;

  //#pragma HLS dependence variable=hashmap inter WAR false
  //#pragma HLS dependence variable=hashmap inter RAW false

#pragma HLS pipeline II=1

  if (!new_rpc_s.in.empty()) {
    new_rpc_t new_rpc = new_rpc_s.in.read();

    // Is this a request message?
    if (new_rpc.id == 0) {
      // Get a unique local ID for this peer
      peer_hashpack_t query = {new_rpc.dest_addr.sin6_addr.s6_addr};
      new_rpc.peer_id = hashmap.search(query);

      // Search for peer failed, create it
      if (new_rpc.peer_id == 0) {
	new_rpc.peer_id = peer_stack.pop();

	entry_t<peer_hashpack_t, peer_id_t> entry = {query, new_rpc.peer_id};
	hashmap.queue(entry);
      }
    }

    new_rpc_s.out.write(new_rpc);
  } else {
    hashmap.process();
  }

  //else if (!onboard_rpc_insert_in.empty()) {
  //  onboard_rpc_t onboard_rpc = onboard_rpc_insert_in.read();

  //  peer_hashpack_t hashpack = {onboard_rpc.daddr};

  //  entry_t<peer_hashpack_t, peer_id_t> entry = {hashpack, insertion.peer_id};
  //  hashmap.queue(entry);

  //  onboard_rpc_insert_out.write(onboard_rpc);
  //}

  //if (!peer_table_request_primary.empty()) {
  //  peer_hashpack_t query;
  //  peer_table_request_primary.read(query);

  //  peer_id_t peer_id = hashmap.search(query);

  //  peer_table_response_primary.write(peer_id);
  //} else if (!peer_table_request_secondary.empty()) {
  //  peer_hashpack_t query;
  //  peer_table_request_secondary.read(query);

  //  peer_id_t peer_id = hashmap.search(query);

  //  peer_table_response_secondary.write(peer_id);
  //} else if (!peer_table_insert_primary.empty()) {
  //  homa_peer_t insertion;
  //  peer_table_insert_primary.read(insertion);

  //  peer_hashpack_t hashpack = {insertion.addr};

  //  entry_t<peer_hashpack_t, peer_id_t> entry = {hashpack, insertion.peer_id};
  //  hashmap.queue(entry);
  //} else if (!peer_table_insert_secondary.empty()) {
  //  homa_peer_t insertion;
  //  peer_table_insert_secondary.read(insertion);

  //  peer_hashpack_t hashpack = {insertion.addr};

  //  entry_t<peer_hashpack_t, peer_id_t> entry = {hashpack, insertion.peer_id};
  //  hashmap.queue(entry);
}

//void peer_state(stream_t<new_rpc_t, new_rpc_t> & new_rpc_t) {
//
//#pragma HLS pipeline II=1
//
//  static homa_peer_t peers[MAX_PEERS];
//
//  peer_id_t query;
//
//  if (!onboard_rpc_in.empty()) {
//    onboard_rpc_t onboard_rpc = onboard_rpc_in.read();
//    homa_peer_t peer;
//
//    peer.peer_id = onboard_rpc.peer_id;
//    peer.addr = onboard_rpc.dest_addr.sin6_addr.s6_addr;
//
//    onboard_rpc_out.write(onboard_rpc);
//    // Store the mapping from IP addres to peer
//    //peer_table_insert.write(peer);
//  }
//
//  //  if (peer_buffer_request_primary.read_nb(query)) {
//  //    peer_buffer_response_primary.write(peers[query-1]);
//  //  } 
//
//  homa_peer_t update;
//  if (peer_buffer_insert_primary.read_nb(update)) {
//    peers[update.peer_id-1] = update;
//  } else if (peer_buffer_insert_secondary.read_nb(update)) {
//    peers[update.peer_id-1] = update;
//  }
//}



//void update_peer_stack(hls::stream<onboard_rpc_t> & onboard_rpc_in,
//		       hls::stream<onboard_rpc_t> & onboard_rpc_out,
//		       hls::stream<peer_id_t> & peer_stack_free) {
//		       //hls::stream<peer_id_t> & peer_stack_next_primary,
//		       //hls::stream<peer_id_t> & peer_stack_next_secondary,
//		       //hls::stream<peer_id_t> & peer_stack_free) {
//
//#pragma HLS pipeline II=1
//
//  static stack_t<peer_id_t, MAX_PEERS> peer_stack(true);
//
//  if (!peer_stack.empty() && !onboard_rpc_in.empty()) {
//    onboard_rpc_t onboard_rpc = onboard_rpc_in.read();
//    onboard_rpc.peer_id = peer_stack.pop();
//    onboard_rpc_out.write(onboard_rpc);
//    //if (!rpc_stack_next_primary.full()) {
//    //rpc_stack_next_primary.write((next_rpc + 1) << 1);
//    //} //else if (!rpc_stack_next_secondary.full()) {
//      //std::cerr << "write rpc_stack_next_secondary\n";
//      //rpc_stack_next_secondary.write((next_rpc + 1) << 1);
//    //}
//  } else if (!peer_stack_free.empty()) {
//    peer_id_t freed_peer = peer_stack_free.read();
//    rpc_stack.push(freed_rpc - 1);
//  }
//
//  //if (!peer_stack.empty()) {
//  //  if (!peer_stack_next_primary.full()) {
//  //    peer_id_t next_peer = peer_stack.pop();
//  //    peer_stack_next_primary.write(next_peer+1);
//  //  } else if(!peer_stack_next_secondary.full()) {
//  //    peer_id_t next_peer = peer_stack.pop();
//  //    peer_stack_next_secondary.write(next_peer+1);
//  //  }
//  //}
//
//  //peer_id_t freed_peer;
//  //if (peer_stack_free.read_nb(freed_peer)) {
//  //  peer_stack.push(freed_peer-1);
//  //}
//}

