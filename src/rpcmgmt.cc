#include "rpcmgmt.hh"
#include "hashmap.hh"

/**
 * update_rpc_stack() -  ....
 */
void update_rpc_stack(hls::stream<rpc_id_t> & rpc_stack_next_primary,
		      hls::stream<rpc_id_t> & rpc_stack_next_secondary,
		      hls::stream<rpc_id_t> & rpc_stack_free) {

  static stack_t<rpc_id_t, MAX_RPCS> rpc_stack(true);

#pragma HLS pipeline II=1

  rpc_id_t freed_rpc;

  /*
    RPC 0 is the null rpc (hence the +1)
    Always return an even RPC ID, so that it may be converted to a server
  */
  
  if (!rpc_stack.empty()) {
    rpc_id_t next_rpc = rpc_stack.pop();
    if (!rpc_stack_next_primary.full()) {
      rpc_stack_next_primary.write((next_rpc + 1) << 1);
    } else if (!rpc_stack_next_secondary.full()) {
      rpc_stack_next_secondary.write((next_rpc + 1) << 1);
    }
  } 

  if (rpc_stack_free.read_nb(freed_rpc)) {
    rpc_stack.push((freed_rpc >> 1) - 1);
  }
}

/**
 * update_rpc_table() -  ....
 *
 * TODO: add support for deletion
 */
void update_rpc_table(hls::stream<rpc_hashpack_t> & rpc_table_request_primary,
		      hls::stream<rpc_id_t> & rpc_table_response_primary,
		      hls::stream<rpc_hashpack_t> & rpc_table_request_secondary,
		      hls::stream<rpc_id_t> & rpc_table_response_secondary,
     		      hls::stream<homa_rpc_t> & rpc_table_insert) {
  static hashmap_t<rpc_hashpack_t, rpc_id_t, RPC_SUB_TABLE_SIZE, RPC_SUB_TABLE_INDEX, RPC_HP_SIZE, MAX_OPS> hashmap;
  // TODO check these deps
#pragma HLS dependence variable=hashmap inter WAR false
#pragma HLS dependence variable=hashmap inter RAW false

#pragma HLS pipeline II=1

  if (!rpc_table_request_primary.empty()) {
    rpc_hashpack_t query;
    rpc_table_request_primary.read(query);

    rpc_id_t rpc_id = hashmap.search(query);

    rpc_table_response_primary.write(rpc_id);
  } else if (!rpc_table_request_secondary.empty()) {
    rpc_hashpack_t query;
    rpc_table_request_secondary.read(query);

    rpc_id_t rpc_id = hashmap.search(query);

    rpc_table_response_secondary.write(rpc_id);
  } else if (!rpc_table_insert.empty()) {
    homa_rpc_t insertion;
    rpc_table_insert.read(insertion);

    rpc_hashpack_t hashpack = {insertion.daddr, insertion.rpc_id, insertion.dport, 0};

    entry_t<rpc_hashpack_t, rpc_id_t> entry = {hashpack, insertion.rpc_id};
    hashmap.queue(entry);
  } else {
    hashmap.process();
  }
}

void update_rpc_buffer(hls::stream<rpc_id_t> & rpc_buffer_request_primary,
		       hls::stream<homa_rpc_t> & rpc_buffer_response_primary,
		       hls::stream<pending_pkt_t> & pending_pkt_in,
		       hls::stream<pending_pkt_t> & pending_pkt_out,
		       hls::stream<rpc_id_t> & rpc_buffer_request_ternary,
		       hls::stream<homa_rpc_t> & rpc_buffer_response_ternary,
		       hls::stream<homa_rpc_t> & rpc_buffer_insert_primary,
		       hls::stream<homa_rpc_t> & rpc_buffer_insert_secondary) {
  // Actual RPC data
  static homa_rpc_t rpcs[MAX_RPCS];
#pragma HLS dependence variable=rpcs inter WAR false
#pragma HLS dependence variable=rpcs inter RAW false

#pragma HLS pipeline II=1

  rpc_id_t query;
  homa_rpc_t update;
  pending_pkt_t pending_pkt;

  if (rpc_buffer_request_primary.read_nb(query)) {
    rpc_buffer_response_primary.write(rpcs[(query >> 1)-1]);
  } else if (pending_pkt_in.read_nb(pending_pkt)) {
    homa_rpc_t homa_rpc = rpcs[(pending_pkt.rpc_id >> 1)-1];
    pending_pkt.xmit_id = homa_rpc.msgout.xmit_id;
    pending_pkt.daddr = homa_rpc.daddr;
    pending_pkt.dport = homa_rpc.dport;
    pending_pkt.saddr = homa_rpc.saddr;
    pending_pkt.sport = homa_rpc.sport;
    pending_pkt_out.write(pending_pkt);
  }
  // TODO may need to segment up homa_rpc
  //else if (rpc_buffer_request_ternary.read_nb(query)) {
  //rpc_buffer_response_ternary.write(rpcs[(query >> 1)-1]);
  //}
  rpc_id_t rpc_id;
  rpc_buffer_request_ternary.read_nb(rpc_id);
  homa_rpc_t dummy;
  rpc_buffer_response_ternary.write_nb(dummy);

  if (rpc_buffer_insert_primary.read_nb(update)) {
    rpcs[(update.rpc_id >> 1)-1] = update;
  } else if (rpc_buffer_insert_secondary.read_nb(update)) {
    rpcs[(update.rpc_id >> 1)-1] = update;
  } 
}
