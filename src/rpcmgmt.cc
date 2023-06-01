#include "rpcmgmt.hh"
#include "hashmap.hh"

/**
 * update_rpc_stack() -  ....
 */
//void update_rpc_stack(hls::stream<onboard_rpc_t> & onboard_rpc_in,
//		      hls::stream<onboard_rpc_t> & onboard_rpc_out,
//		      hls::stream<rpc_id_t> & rpc_stack_free) {
//
//#pragma HLS pipeline II=1
//
//  static stack_t<rpc_id_t, MAX_RPCS> rpc_stack(true);
//
//  /*
//    RPC 0 is the null rpc (hence the +1)
//    Always return an even RPC ID, so that it may be converted to a server
//  */
//  
//  if (!rpc_stack.empty() && !onboard_rpc_in.empty()) {
//    onboard_rpc_t onboard_rpc = onboard_rpc_in.read();
//    onboard_rpc.rpc_id = rpc_stack.pop();
//    onboard_rpc_out.write(onboard_rpc);
//    //if (!rpc_stack_next_primary.full()) {
//    //rpc_stack_next_primary.write((next_rpc + 1) << 1);
//    //} //else if (!rpc_stack_next_secondary.full()) {
//      //std::cerr << "write rpc_stack_next_secondary\n";
//      //rpc_stack_next_secondary.write((next_rpc + 1) << 1);
//    //}
//  } else if (!rpc_stack_free.empty()) {
//    rpc_id_t freed_rpc = rpc_stack_free.read();
//    rpc_stack.push((freed_rpc >> 1) - 1);
//  }
//}

/**
 * update_rpc_table() -  ....
 *
 * TODO: add support for deletion
 */
void update_rpc_table(stream_t<new_rpc_t, new_rpc_t> & new_rpc_s) {

		      //hls::stream<onboard_rpc_t> & onboard_rpc_in,
		      //hls::stream<onboard_rpc_t> & onboard_rpc_out) {
		      //hls::stream<rpc_hashpack_t> & rpc_table_request_primary,
		      //hls::stream<rpc_id_t> & rpc_table_response_primary,
		      //hls::stream<rpc_hashpack_t> & rpc_table_request_secondary,
		      //hls::stream<rpc_id_t> & rpc_table_response_secondary,
     		      //hls::stream<homa_rpc_t> & rpc_table_insert) {

  static hashmap_t<rpc_hashpack_t, rpc_id_t, RPC_SUB_TABLE_SIZE, RPC_SUB_TABLE_INDEX, RPC_HP_SIZE, MAX_OPS> hashmap;
#pragma HLS dependence variable=hashmap inter WAR false
#pragma HLS dependence variable=hashmap inter RAW false

#pragma HLS pipeline II=1

  if (!onboard_rpc_in.empty()) {
    onboard_rpc_t onboard_rpc = onboard_rpc_in.read();

    if (onboard_rpc.id == 0) {
      // Get local ID from address, ID, and port
      rpc_hashpack_t query = {onboard_rpc.dest_addr.sin6_addr.s6_addr, onboard_rpc.rpc_id, onboard_rpc.dest_addr.sin6_port, 0};

      onboard_rpc.rpc_id = hashmap.search(query);
    }
    onboard_rpc_out.write(onboard_rpc);
  }

  //if (!rpc_table_request_primary.empty()) {
  //  rpc_hashpack_t query;
  //  rpc_table_request_primary.read(query);

  //  rpc_id_t rpc_id = hashmap.search(query);

  //  rpc_table_response_primary.write(rpc_id);
  //} else if (!rpc_table_request_secondary.empty()) {
  //  rpc_hashpack_t query;
  //  rpc_table_request_secondary.read(query);

  //  rpc_id_t rpc_id = hashmap.search(query);

  //  rpc_table_response_secondary.write(rpc_id);
  //} else if (!rpc_table_insert.empty()) {
  //  homa_rpc_t insertion;
  //  rpc_table_insert.read(insertion);

  //  rpc_hashpack_t hashpack = {insertion.daddr, insertion.rpc_id, insertion.dport, 0};

  //  entry_t<rpc_hashpack_t, rpc_id_t> entry = {hashpack, insertion.rpc_id};
  //  hashmap.queue(entry);
  } else {
    hashmap.process();
  }
}

void rpc_state(stream_t<new_rpc_t, new_rpc_t> & new_rpc_s,
	       stream_t<out_pkt_t, out_pkt_t> & out_pkt_s) {

  static stack_t<rpc_id_t, MAX_RPCS> rpc_stack(true);
  static homa_rpc_t rpcs[MAX_RPCS]; // TODO annotate as dual port

#pragma HLS pipeline II=1
#pragma HLS dependence variable=rpcs inter WAR false
#pragma HLS dependence variable=rpcs inter RAW false

  /* Read only process */
  if (!out_pkt_s.in.empty()) {
    out_pkt_t out_pkt = out_pkt_s.in.read();
    homa_rpc_t homa_rpc = rpcs[(out_pkt.rpc_id >> 1)-1];

    out_pkt.xmit_id = homa_rpc.msgout.xmit_id;
    out_pkt.daddr = homa_rpc.daddr;
    out_pkt.dport = homa_rpc.dport;
    out_pkt.saddr = homa_rpc.saddr;
    out_pkt.sport = homa_rpc.sport;

    out_pkt_s.out.write(out_pkt);
  }

  if (!new_rpc_s.in.empty()) {
    new_rpc_t new_rpc = new_rpc_s.in.read();

    if (new_rpc.id != 0) { // Is this a request message?
      new_rpc.rpc_id = rpc_stack.pop();
      homa_rpc_t homa_rpc;
      homa_rpc.state = homa_rpc_t::RPC_OUTGOING;
      homa_rpc.rpc_id = new_rpc.rpc_id;
      homa_rpc.daddr = params->dest_addr.sin6_addr.s6_addr;
      homa_rpc.dport = params->dest_addr.sin6_port;
      homa_rpc.saddr = params->src_addr.sin6_addr.s6_addr;
      homa_rpc.sport = params->src_addr.sin6_port;
      homa_rpc.buffout = params->buffout;
      homa_rpc.buffin = params->buffin;
      homa_rpc.msgout.length = params->length;

      homa_rpc.msgout.unscheduled = homa->rtt_bytes;
      if (homa_rpc.msgout.unscheduled > homa_rpc.msgout.length)
	homa_rpc.msgout.unscheduled = homa_rpc.msgout.length;

      rpcs[(new_rpc.rpc_id >> 1)-1] = homa_rpc;
    } else { // Is this a response message?
      homa_rpc_t homa_rpc = rpcs[(new_rpc.rpc_id >> 1)-1];
      homa_rpc.state = homa_rpc_t::RPC_OUTGOING;

      homa_rpc.msgout.unscheduled = homa->rtt_bytes;
      if (homa_rpc.msgout.unscheduled > homa_rpc.msgout.length)
	homa_rpc.msgout.unscheduled = homa_rpc.msgout.length;

      rpcs[(new_rpc.rpc_id >> 1)-1] = homa_rpc;
    }

    new_rpc_s.out.write(new_rpc);
  }

  // TODO if in_pkt_s .... else if new_rpc_s

  //else if (!onboard_rpc_req_new_peer_in.empty()) {
  //  onboard_rpc_t onboard_rpc = onboard_rpc_req_new_peer_in.read();
  //  homa_rpc_t homa_rpc = rpcs[(onboard_rpc.rpc_id >> 1)-1];

  //  homa_rpc.completion_cookie = onboard_rpc.completion_cookie;
  //  homa_rpc.state = homa_rpc_t::RPC_OUTGOING;
  //  homa_rpc.daddr = onboard_rpc.dest_addr.sin6_addr.s6_addr;
  //  homa_rpc.dport = onboard_rpc.dest_addr.sin6_port;
  //  homa_rpc.saddr = onboard_rpc.src_addr.sin6_addr.s6_addr;
  //  homa_rpc.sport = onboard_rpc.src_addr.sin6_port;
  //  homa_rpc.buffout = onboard_rpc.buffout;
  //  homa_rpc.buffin = onboard_rpc.buffin;

  //  rpcs[(onboard_rpc.rpc_id >> 1)-1] = homa_rpc;

  //  int granted = rtt_bytes;
  //  if (rtt_bytes > onboard_rpc.length)
  //    granted = onboard_rpc.length;

  //  onboard_rpc.granted = granted;

  //  onboard_rpc_out.write(onboard_rpc);
  //} else if (!onboard_rpc_req_in.empty()) {
  //  onboard_rpc_t onboard_rpc = onboard_rpc_req_in.read();
  //  homa_rpc_t homa_rpc = rpcs[(onboard_rpc.rpc_id >> 1)-1];

  //  homa_rpc.completion_cookie = onboard_rpc.completion_cookie;
  //  homa_rpc.state = homa_rpc_t::RPC_OUTGOING;
  //  homa_rpc.daddr = onboard_rpc.dest_addr.sin6_addr.s6_addr;
  //  homa_rpc.dport = onboard_rpc.dest_addr.sin6_port;
  //  homa_rpc.saddr = onboard_rpc.src_addr.sin6_addr.s6_addr;
  //  homa_rpc.sport = onboard_rpc.src_addr.sin6_port;
  //  homa_rpc.buffout = onboard_rpc.buffout;
  //  homa_rpc.buffin = onboard_rpc.buffin;

  //  rpcs[(onboard_rpc.rpc_id >> 1)-1] = homa_rpc;

  //  int granted = rtt_bytes;
  //  if (rtt_bytes > onboard_rpc.length)
  //    granted = onboard_rpc.length;

  //  onboard_rpc.granted = granted;

  //  onboard_rpc_out.write(onboard_rpc);
  //} else if (!onboard_rpc_resp_in.empty()) {
  //  onboard_rpc_t onboard_rpc = onboard_rpc_resp_in.read();
  //  homa_rpc_t homa_rpc = rpcs[(onboard_rpc.rpc_id >> 1)-1];

  //  homa_rpc.state = homa_rpc_t::RPC_OUTGOING;
  //  rpcs[(onboard_rpc.rpc_id >> 1)-1] = homa_rpc;

  //  int granted = rtt_bytes;
  //  if (rtt_bytes > onboard_rpc.length)
  //    granted = onboard_rpc.length;

  //  onboard_rpc.granted = granted;
  //  
  //  onboard_rpc_out.write(onboard_rpc);
  //}
 
  //if (rpc_buffer_insert_primary.read_nb(update)) {
  //  rpcs[(update.rpc_id >> 1)-1] = update;
  //} else if (rpc_buffer_insert_secondary.read_nb(update)) {
  //  rpcs[(update.rpc_id >> 1)-1] = update;
  //} 
}
