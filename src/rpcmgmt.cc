#include "rpcmgmt.hh"
#include "peer.hh"

homa_rpc_t rpcs[MAX_RPCS];


homa_rpc_stack_t rpc_stack;

/**
 * homa_rpc_new_client() - Construct a client RPC (one that is used
 * to issue an outgoing request). Doesn't send any packets. 
 * @hsk:      Socket to which the RPC belongs.
 * @dest:     Address of host (ip and port) to which the RPC will be sent.
 *
 * Return:    A pointer to the respective RPC
 */
homa_rpc_t & homa_rpc_new_client(homa_t * homa, int & output_offset, in6_addr & dest_addr) {
  // Grab an availible RPC
  homa_rpc_id_t rpc_id = rpc_stack.pop();
  homa_rpc_t & crpc = rpcs[rpc_id];

  crpc.output_offset = output_offset;
  crpc.id = rpc_id;
  crpc.state = homa_rpc_t::RPC_OUTGOING;
  crpc.grants_in_progress = 0;

  crpc.peer = homa_peer_find(dest_addr);

  crpc.completion_cookie = 0;
  crpc.msgin.total_length = -1;
  crpc.msgout.length = -1;
  crpc.silent_ticks = 0;
  crpc.resend_timer_ticks = homa->timer_ticks;
  crpc.done_timer_ticks = 0;

  return crpc;
}

/**
 * homa_rpc_find() - 
 * @id:       Unique identifier for the RPC (must have server bit set).
 *
 * Return:    
 */
homa_rpc_t & homa_rpc_find(ap_uint<64> id) {
  return rpcs[id];
}

