#include "rpcmgmt.hh"
#include "hashmap.hh"
#include <iostream>

void rpc_state(hls::stream<new_rpc_t> & peer_map__rpc_state,
	       hls::stream<new_rpc_t> & rpc_state__srpt_data,
	       hls::stream<out_pkt_t> & egress_sel__rpc_state, 
	       hls::stream<out_pkt_t> & rpc_state__chunk_dispatch) {

  static stack_t<rpc_id_t, MAX_RPCS> rpc_stack(true);
  static homa_rpc_t rpcs[MAX_RPCS]; // TODO annotate as dual port

#pragma HLS pipeline II=1
#pragma HLS dependence variable=rpcs inter WAR false
#pragma HLS dependence variable=rpcs inter RAW false

  /* Read only process */
  if (!egress_sel__rpc_state.empty()) {

    out_pkt_t out_pkt = egress_sel__rpc_state.read();
    homa_rpc_t homa_rpc = rpcs[(out_pkt.rpc_id >> 1)-1];

    out_pkt.dbuff_id = homa_rpc.msgout.dbuff_id;
    out_pkt.daddr = homa_rpc.daddr;
    out_pkt.dport = homa_rpc.dport;
    out_pkt.saddr = homa_rpc.saddr;
    out_pkt.sport = homa_rpc.sport;

    rpc_state__chunk_dispatch.write(out_pkt);
  }

  if (!peer_map__rpc_state.empty()) {

    //std::cerr << "DEBUG: RPC Store" << std::endl;
    DEBUG_MSG("RPC Store")

    new_rpc_t new_rpc = peer_map__rpc_state.read();

    if (new_rpc.id == 0) { // Is this a request message?
      std::cerr << "REQUEST MESSAGE\n";
      new_rpc.local_id = rpc_stack.pop();
      homa_rpc_t homa_rpc;
      homa_rpc.state = homa_rpc_t::RPC_OUTGOING;
      homa_rpc.rpc_id = new_rpc.local_id;
      homa_rpc.daddr = new_rpc.dest_addr.sin6_addr.s6_addr;
      homa_rpc.dport = new_rpc.dest_addr.sin6_port;
      homa_rpc.saddr = new_rpc.src_addr.sin6_addr.s6_addr;
      homa_rpc.sport = new_rpc.src_addr.sin6_port;
      homa_rpc.buffout = new_rpc.buffout;
      homa_rpc.buffin = new_rpc.buffin;
      homa_rpc.msgout.length = new_rpc.length;
      homa_rpc.msgout.dbuff_id = new_rpc.dbuff_id;

      rpcs[(new_rpc.local_id >> 1)-1] = homa_rpc;
    } else { // Is this a response message?
      homa_rpc_t homa_rpc = rpcs[(new_rpc.local_id >> 1)-1];
      homa_rpc.state = homa_rpc_t::RPC_OUTGOING;
      homa_rpc.msgout.dbuff_id = new_rpc.dbuff_id;

      rpcs[(new_rpc.local_id >> 1)-1] = homa_rpc;
    }

    rpc_state__srpt_data.write(new_rpc);
  }
}
