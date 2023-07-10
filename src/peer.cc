#include "peer.hh"
#include "stack.hh"
#include "hashmap.hh"
#include <iostream>

void peer_map(hls::stream<sendmsg_t, VERIF_DEPTH> & sendmsg_i,
	      hls::stream<sendmsg_t, VERIF_DEPTH> & sendmsg_o,
	      hls::stream<recvmsg_t, VERIF_DEPTH> & recvmsg_i,
	      hls::stream<recvmsg_t, VERIF_DEPTH> & recvmsg_o) {

  static stack_t<peer_id_t, MAX_PEERS> peer_stack(true);
  static hashmap_t<peer_hashpack_t, peer_id_t, PEER_SUB_TABLE_SIZE, PEER_SUB_TABLE_INDEX, PEER_HP_SIZE, MAX_OPS> hashmap;

//#pragma HLS dependence variable=peer_stack inter WAR false
//#pragma HLS dependence variable=peer_stack inter RAW false
//#pragma HLS dependence variable=peer_stack intra WAR false
//#pragma HLS dependence variable=peer_stack intra RAW false
//
//
//#pragma HLS dependence variable=hashmap inter WAR false
//#pragma HLS dependence variable=hashmap inter RAW false
//#pragma HLS dependence variable=hashmap intra WAR false
//#pragma HLS dependence variable=hashmap intra RAW false

// #pragma HLS dependence variable=hashmap inter WAR false

  // TODO Can this be problematic?
// #pragma HLS dependence variable=hashmap inter WAR false
// #pragma HLS dependence variable=hashmap inter RAW false

// #pragma HLS pipeline style=flp
#pragma HLS pipeline II=1 style=flp

  //if (!header_in_i.empty()) {
  //  std::cerr << "header in peer map\n";
  //  header_t header_in = header_in_i.read();

  //  peer_hashpack_t query = {header_in.saddr};

  //  peer_id_t peer_id = hashmap.search(query);

  //  header_in.peer_id = peer_id;

  //  //std::cerr << "header out " << peer_id << std::endl;
  //  header_in_o.write(header_in);
  //} else
  sendmsg_t sendmsg;
  recvmsg_t recvmsg;
  if (sendmsg_i.read_nb(sendmsg)) {

     peer_hashpack_t query = {sendmsg.daddr};
     sendmsg.peer_id = hashmap.search(query);

     if (sendmsg.peer_id == 0) {
        sendmsg.peer_id = peer_stack.pop();

        entry_t<peer_hashpack_t, peer_id_t> entry = {query, sendmsg.peer_id};
        hashmap.queue(entry);
     }
     sendmsg_o.write_nb(sendmsg);
  } else if (recvmsg_i.read_nb(recvmsg)) {

     peer_hashpack_t query = {recvmsg.daddr};
     recvmsg.peer_id = hashmap.search(query);

     if (recvmsg.peer_id == 0) {
        recvmsg.peer_id = peer_stack.pop();

        entry_t<peer_hashpack_t, peer_id_t> entry = {query, recvmsg.peer_id};
        hashmap.queue(entry);
     }

     recvmsg_o.write_nb(recvmsg);
  } else {
     hashmap.process();
  }
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

