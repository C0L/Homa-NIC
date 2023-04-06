#include "rpcmgmt.hh"

static homa_rpc_t rpcs[MAX_RPCS];
static homa_rpc_stack_t rpc_stack;

/**
 * homa_rpc_new_client() - Construct a client RPC (one that is used
 * to issue an outgoing request). Doesn't send any packets. 
 * @hsk:      Socket to which the RPC belongs.
 * @dest:     Address of host (ip and port) to which the RPC will be sent.
 *
 * Return:    A pointer to the respective RPC
 */
homa_rpc_id_t homa_rpc_new_client(int & output_offset, ap_uint<64> & dest_addr) {
  // Grab an availible RPC
  homa_rpc_id_t rpc_id = rpc_stack.pop();
  homa_rpc_t & crpc = rpcs[rpc_id];

  crpc.output_offset = output_offset;
  cprc.id = rpc_id;
  crpc.state = RPC_OUTGOING;
  crpc.grants_in_progress = 0;
  crpc.peer = homa_peer_find(&dest_addr_as_ipv6, &hsk->inet); 
  crpc.dport = ntohs(dest->in6.sin6_port);
  crpc.completion_cookie = 0;
  crpc.msgin.total_length = -1;
  crpc->msgout.length = -1;
  crpc->silent_ticks = 0;
  crpc->resend_timer_ticks = hsk->homa->timer_ticks;
  crpc->done_timer_ticks = 0;

  return crpc;
}
