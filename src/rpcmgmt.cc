#include "rpcmgmt.hh"
#include "hashmap.hh"
#include <iostream>

void rpc_state(hls::stream<sendmsg_t> & sendmsg_i,
	       hls::stream<sendmsg_t> & sendmsg_o,
	       hls::stream<recvmsg_t> & recvmsg_i,
	       hls::stream<recvmsg_t> & recvmsg_o,
	       hls::stream<header_t> & header_out_i, 
	       hls::stream<header_t> & header_out_o,
	       hls::stream<header_t> & header_in_i,
	       hls::stream<header_t> & header_in_o,
	       hls::stream<srpt_grant_t> & srpt_grant_o) {

  static stack_t<rpc_id_t, MAX_RPCS> rpc_stack(true);
  static homa_rpc_t rpcs[MAX_RPCS];

#pragma HLS pipeline II=1
#pragma HLS dependence variable=rpcs inter WAR false
#pragma HLS dependence variable=rpcs inter RAW false

  /* Read only process */
  if (!header_out_i.empty()) {
    header_t header_out = header_out_i.read();
    homa_rpc_t homa_rpc = rpcs[(header_out.local_id >> 1)-1];

    header_out.dbuff_id = homa_rpc.msgout.dbuff_id;
    header_out.daddr = homa_rpc.daddr;
    header_out.dport = homa_rpc.dport;
    header_out.saddr = homa_rpc.saddr;
    header_out.sport = homa_rpc.sport;


    header_out_o.write(header_out);
  }

  if (!header_in_i.empty()) {
    header_t header_in = header_in_i.read();

    homa_rpc_t homa_rpc = rpcs[(header_in.local_id >> 1)-1];

    if (header_in.type == DATA) {
      header_in.dma_offset = homa_rpc.buffout;
      // TODO maybe need some other info depending on packet type?
      header_in_o.write(header_in);
    } else if (header_in.type == GRANT) {
      srpt_grant_t srpt_grant;
      srpt_grant_o.write(srpt_grant);
    }
  }

  /* R/W Processes */
  if (!sendmsg_i.empty()) {
    sendmsg_t sendmsg = sendmsg_i.read();

    if (sendmsg.id == 0) { // Is this a request message?
      sendmsg.local_id = rpc_stack.pop();

      homa_rpc_t homa_rpc;
      homa_rpc.state = homa_rpc_t::RPC_OUTGOING;
      homa_rpc.daddr = sendmsg.daddr;
      homa_rpc.dport = sendmsg.dport;
      homa_rpc.saddr = sendmsg.saddr;
      homa_rpc.sport = sendmsg.sport;
      homa_rpc.buffin = sendmsg.buffin;
      homa_rpc.msgout.length = sendmsg.length;
      homa_rpc.msgout.dbuff_id = sendmsg.dbuff_id;

      rpcs[(sendmsg.local_id >> 1)-1] = homa_rpc;
    } else { // Is this a response message?
      homa_rpc_t homa_rpc = rpcs[(sendmsg.local_id >> 1)-1];
      homa_rpc.state = homa_rpc_t::RPC_OUTGOING;
      homa_rpc.msgout.dbuff_id = sendmsg.dbuff_id;

      rpcs[(sendmsg.local_id >> 1)-1] = homa_rpc;
    }

    sendmsg_o.write(sendmsg);
  } else if (!recvmsg_i.empty()) {
    recvmsg_t recvmsg = recvmsg_i.read();

    // If the caller ID determines if this machine is client or server
    recvmsg.local_id = (recvmsg.id == 0) ? rpc_stack.pop() : (rpc_id_t) recvmsg.id;

    homa_rpc_t homa_rpc;
    homa_rpc.state = homa_rpc_t::RPC_INCOMING;
    //homa_rpc.local_id = sendmsg.local_id;
    homa_rpc.daddr = recvmsg.daddr;
    homa_rpc.dport = recvmsg.dport;
    homa_rpc.saddr = recvmsg.saddr;
    homa_rpc.sport = recvmsg.sport;
    homa_rpc.buffout = recvmsg.buffout;


    rpcs[(recvmsg.local_id >> 1)-1] = homa_rpc;
    recvmsg_o.write(recvmsg);
  }
}

void rpc_map(hls::stream<header_t> & header_in_i,
	     hls::stream<header_t> & header_in_o,
	     hls::stream<recvmsg_t> & recvmsg_i) {

  static hashmap_t<rpc_hashpack_t, rpc_id_t, RPC_SUB_TABLE_SIZE, RPC_SUB_TABLE_INDEX, RPC_HP_SIZE, MAX_OPS> hashmap;

  // TODO can this be problematic?
#pragma HLS dependence variable=hashmap inter WAR false
#pragma HLS dependence variable=hashmap inter RAW false

#pragma HLS pipeline II=1

  if (!header_in_i.empty()) {
    header_t header_in = header_in_i.read();

    /* The incoming message is a request */
    if (!IS_CLIENT(header_in.sender_id)) {

      // Unscheduled packets need to be mapped to tmp recv entry
      if (header_in.type == DATA) {
	if (header_in.data_offset == 0) {
	  rpc_hashpack_t recvfind = {header_in.daddr, 0, header_in.dport, 0};

	  header_in.local_id = hashmap.search(recvfind);

	  rpc_hashpack_t recvspecialize = {header_in.daddr, header_in.sender_id, header_in.dport, 0};

	  hashmap.queue({recvspecialize , header_in.local_id});
	} else {
	  rpc_hashpack_t recvspecialize = {header_in.daddr, header_in.sender_id, header_in.dport, 0};

	  header_in.local_id = hashmap.search(recvspecialize);
	}
      } else {
	rpc_hashpack_t query = {header_in.daddr, header_in.sender_id, header_in.dport, 0};

	header_in.local_id = hashmap.search(query);
      }
    }

    // If the incoming message is a response, the RPC ID is already valid in the local store
    header_in_o.write(header_in);
  } else if (!recvmsg_i.empty()) {
    recvmsg_t recvmsg = recvmsg_i.read();

    // Create a mapping for incoming data
    rpc_hashpack_t query = {recvmsg.daddr, recvmsg.id, recvmsg.dport, 0};

    hashmap.queue({query, recvmsg.local_id}); 
  } else {
    hashmap.process();
  }
}
