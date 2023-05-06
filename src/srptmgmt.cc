#include "srptmgmt.hh"
#include "rpcmgmt.hh"

/**
 * update_xmit_srpt_queue() - Determines what message to transmit next.
 * @srpt_queue_insert: Injest path for new messages to xmit (unscheduled bytes)
 * @srpt_queue_grant: Updates for RPCs when more bytes are granted
 * @srpt_queue_next:   The highest priority value to transmit
 */
void update_xmit_srpt_queue(hls::stream<srpt_xmit_entry_t> & srpt_queue_insert,
			    hls::stream<srpt_xmit_entry_t> & srpt_queue_grants,
			    hls::stream<srpt_xmit_entry_t> & srpt_queue_next) {
  static srpt_queue_t<srpt_xmit_entry_t, MAX_SRPT> srpt_queue;

  static uint32_t grants[MAX_RPCS];
  static uint32_t remainings[MAX_RPCS];

  // The RAM needs three ports aparently
  #pragma HLS bind_storage variable=grants type=RAM_1WNR
  #pragma HLS bind_storage variable=remainings type=RAM_1WNR

  #pragma HLS dependence variable=grants inter RAW false
  #pragma HLS dependence variable=remainings inter RAW false

#pragma HLS pipeline II=1

  if (!srpt_queue_next.full() && !srpt_queue.empty()) {
    srpt_xmit_entry_t & head = srpt_queue.head();

    uint32_t grant = grants[head.rpc_id];
    uint32_t remaining = remainings[head.rpc_id];

    head.remaining -= PACKET_SIZE;
    grants[head.rpc_id] = grant - PACKET_SIZE;
    remainings[head.rpc_id] = remaining - PACKET_SIZE;

    srpt_queue_next.write(head);
    if (grant <= 0) srpt_queue.pop();
  } else if (!srpt_queue_grants.empty()) {
    srpt_xmit_entry_t new_grant;

    srpt_queue_grants.read(new_grant);

    uint32_t grant = grants[new_grant.rpc_id];
    uint32_t remaining = remainings[new_grant.rpc_id];

    if (grant == 0) {
      new_grant.remaining = remaining;
      grants[new_grant.rpc_id] = remaining;
    } else {
      remainings[new_grant.rpc_id] = new_grant.remaining;
    }

    srpt_queue.push(new_grant);
  } else if (!srpt_queue_insert.empty()) {
    srpt_xmit_entry_t new_entry;

    srpt_queue_insert.read(new_entry);

    uint32_t grant = grants[new_entry.rpc_id];
    uint32_t remaining = remainings[new_entry.rpc_id];

    grants[new_entry.rpc_id] = new_entry.remaining;
    remainings[new_entry.rpc_id] = remaining - PACKET_SIZE;

    srpt_queue.push(new_entry);
  }
}

/**
 * update_grant_srpt_queue() - Determines what message to grant next. Messages are ordered by
 * the least remaining number of grantable bytes. When grants are made, the peer is stored in
 * the active set. Grants will not be issued to peers that are in the active set.
 * @srpt_queue_insert: Injest path for new messages that will need to be granted to.
 * Values originate from link_ingress, which when it sees the unscheduled bytes of
 * a message it has not seen, makes it eligible for granting. 
 * @srpt_queue_next:   The next rpc that should be granted to. Goes to link_egress. 
 */
void update_grant_srpt_queue(hls::stream<srpt_grant_entry_t> & srpt_queue_insert,
			     hls::stream<srpt_grant_entry_t> & srpt_queue_next) {
  /*
   * Ordered based on remaining bytes (and priority: MSG < EMPTY < BLOCKED < ACTIVE)
   * Stores the number of bytes this RPC is eligible to be granted (starts as RTTbytes)
   */
  static srpt_queue_t<srpt_grant_entry_t, MAX_SRPT> srpt_queue;

  // The only thing that can remove elements from active_set is a receipt of packets
  static srpt_grant_entry_t active_set[MAX_OVERCOMMIT];
   
  /*
   * Availible entries in the active set
   */
  static stack_t<peer_id_t, MAX_OVERCOMMIT> active_set_stack(true);
  
  #pragma HLS pipeline II=1
  
  srpt_grant_entry_t new_entry;
  srpt_grant_entry_t receipt;

  int peer_match = -1;

  if (srpt_queue_insert.read_nb(new_entry)) {
    // Is the value in the active set?
    //    for (int i = 0; i < MAX_OVERCOMMIT; ++i) {
    //#pragma HLS unroll
    //      if (active_set[i].peer_id == new_entry.peer_id) {
    //  	peer_match = i;
    //      }
    //    }
    //
    //    // Is this a receipt?
    //    if (peer_match != -1) {
    //      if (active_set[peer_match].rpc_id == new_entry.rpc_id) {
    //	active_set_stack.push(peer_match);
    //
    //	// This receipt indicates the message was fully granted
    //	if (new_entry.granted == 0) {
    //	  new_entry.priority = MSG;
    //	} else {
    //	  new_entry.priority = ACTIVE;
    //	}
    //
    //	srpt_queue.push(new_entry);
    //      } else {
    //	// This is fine because the receipt will unblock this!
    //	new_entry.priority = BLOCKED;
    //	srpt_queue.push(new_entry);
    //      }
    //    } else {
    //      new_entry.priority = ACTIVE;
    //      srpt_queue.push(new_entry);
    //    }
  } else if (!srpt_queue_next.full() && !active_set_stack.empty()) {
    srpt_grant_entry_t top;
    //if (srpt_queue.pop(top)) {
    //      active_set[active_set_stack.pop()] = top;
      srpt_queue_next.write(top);
    //      top.priority = MSG;
    //      srpt_queue.push(top);
    //}
  } // TODO fix and reintroduce

  //else {
    //srpt_queue.order();
  //}
}





  // Do we have a message to xmit and is there room in the output stream
  //if (!srpt_queue_next.full() && !srpt_queue.empty()) {
  //  // Will this rpc not be eligible for broadcast again?
  //  if (srpt_queue.pop(next_rpc)) {
  //    inactive_set[next_rpc.rpc_id] = next_rpc;
  //  }

  //  srpt_queue_next.write(next_rpc);
  //// Otherwise we need to update the remaining bytes and the grantable bytes
  //} else if (srpt_queue_update.read_nb(new_entry)) {
  //  srpt_xmit_entry_t & inactive_entry = inactive_set[new_entry.rpc_id];

  //  // Is this update for a value in the inactive set?
  //  if (inactive_entry.remaining != 0) {
  //    inactive_entry.granted += new_entry.granted;
  //    srpt_queue.push(inactive_entry);
  //  } else {
  //    .. Convert to message?
  //  }
  //// Handle new messages we will need to broadcast
  //}
  ////else

  //if (srpt_queue_insert.read_nb(new_entry)) {
  //  srpt_queue.push(new_entry);
  //}
