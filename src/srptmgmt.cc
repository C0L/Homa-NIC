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
  // TODO can use a smaller FIFO (of pipeline depth) for removing the sync error. After popping from the srpt add it to the blocked RPC BRAM.
  // And add it to the 5 element FIFO. Once it reaches the end of the FIFO, check if the grant has changed, if so readd to SRPT. Otherwise,
  // discard the entry. Need to ensure that a grant does not reactivate and FIFO reactivate.

  static srpt_queue_t<srpt_xmit_entry_t, MAX_SRPT> srpt_queue;
  static uint32_t grants[MAX_RPCS];
  static fifo_t<srpt_xmit_entry_t, MAX_SRPT> exhausted; 


  /*
    Check if the head of the SRPT is not complete but expended all grantable bytes
    If so, then add the entry to the blocked set in BRAM, and add to FIFO
    Once this entry comes to the head of the FIFO then check again if granted has been updated,
    then also checked if the element is still blocked. If the element is no longer blocked, then that
    means the normal process readded the element. If the element is still blocked, yet the granted value
    is updated, that means the new grant and the removal from the SRPT occured at the same time
    and we should readd to the SRPT
  */

#pragma HLS pipeline II=1

  if (!srpt_queue_next.full() && !srpt_queue.empty()) {
    srpt_xmit_entry_t & head = srpt_queue.head();
      
    head.remaining = (head.remaining - PACKET_SIZE < 0) ? 0 : head.remaining - PACKET_SIZE;
    
    srpt_queue_next.write(head);

    uint32_t grant = grants[head.rpc_id];

    // TODO can probably do away with this subtraction step by inverting the value of the grant
    bool ungranted = (head.total - grant == head.remaining);
    bool complete  = (head.remaining == 0);

    if (!complete && ungranted) {
      exhausted.insert(head);
    }
    
    // Are we done sending this message, or are we out of granted bytes?
    if (complete || ungranted) {
      srpt_queue.pop();
    } 
  } else if (!srpt_queue_insert.empty()) {
    srpt_xmit_entry_t new_entry;
    srpt_queue_insert.read(new_entry);
    srpt_queue.push(new_entry);
  } else if (!srpt_queue_grants.empty()) {
    srpt_xmit_entry_t new_grant;
    srpt_queue_grants.read(new_grant);
    grants[new_grant.rpc_id] = new_grant.granted;
  } else if (!exhausted.empty()) {
    srpt_xmit_entry_t exhausted_entry;
    exhausted.remove(exhausted_entry);

    uint32_t grant = grants[exhausted_entry.rpc_id];

    if (grant != exhausted_entry.granted) {
      exhausted_entry.granted = grant;
      srpt_queue.push(exhausted_entry);
    } else {
      exhausted.insert(exhausted_entry);
    }
  }
}

// Some number of unscheduled bytes arrives, the rpc is added with the full length of the message
// minus the unscheduled bytes.

// When this value is added to the active set, the RPC is streamed to the link_egress, which decides
// to grant as much as it pleases. When the results of this grant arrive in link_ingress, the rpc
// is re-added as being grantable. 

/**
 * update_grant_srpt_queue() - Determines what message to grant next. Messages are ordered by
 * increasing number of bytes not yet granted. When grants are made, the peer is
 * stored in the active set. Grants will not be issued to peers that are in the active set.
 * @srpt_queue_insert: Injest path for new messages that will need to be granted to.
 * Values originate from link_ingress, which when it sees the unscheduled bytes of
 * a message it has not seen, makes it eligible for granting. 
 * @srpt_queue_next:   The next rpc that should be granted to. Goes to link_egress. 
 */
void update_grant_srpt_queue(hls::stream<srpt_grant_entry_t> & srpt_queue_insert,
			     //hls::stream<srpt_grant_entry_t> & srpt_queue_receipt,
			     hls::stream<srpt_grant_entry_t> & srpt_queue_next) {

  // TODO This is still troubling. Could always do a tiered brute force search

  static srpt_grant_entry_t active_set[MAX_OVERCOMMIT];
  static stack_t<peer_id_t, MAX_OVERCOMMIT> active_set_stack(true);

  //static next_insert = 0;
  //static fifo_t<srpt_grant_entry_t, 32> insert_fifos[32];
  //static srpt_t<srpt_grant_entry_t, 32> grant_srpts[32]; 

  // New grant insert path

  if (!srpt_queue_insert.empty()) {
    srpt_grant_entry_t new_entry;
    srpt_queue_insert.read(new_entry);

    //grant_srpts[next_insert].push(new_entry);
    //next_insert++;
    srpt_queue_next.write(new_entry);
  }
    //} else if () { // Outgoing path
    //  srpt_xmit_entry_t min_grant;
    //  min_grant.grantable = 0xFFFFFFFF;

    //  // Iterate through every SRPT queue
    //  for (int i = 0; i < 32; ++i) {
    //    for (;;) {
    //	srpt_xmit_entry_t & head = grant_srpts[i].head();

    //	int match = -1;
    //	for (int i = 0; i < MAX_OVERCOMMIT; ++i) {
//#pragma HLS unroll
  //	  if (active_set[i].peer_id == head.peer_id) {
  //	    peer_match = i;
  //	  }
  //	}

  //	if (match != -1) {
  //	  // TODO then we need to bump to the FIFO and try again
  //	  insert_fifo[i].insert(head);
  //	} else {
  //	  min_grant = (min_grant.grantable > head.grantable) ? head : min_grant;
  //	  break;
  //	}

  //	grant_srpts[i].pop();
  //    }
  //  }
  //}

  //  while () {
  //    if (!srpt_queue_insert.empty()) {
  //      srpt_grant_entry_t new_entry;
  //      srpt_queue_insert.read(new_entry);
  //  
  //      int peer_match = -1;
  //  
  //      // Is this peer in use?
  //      for (int i = 0; i < MAX_OVERCOMMIT; ++i) {
  //#pragma HLS unroll
  //	if (active_set[i].peer_id == new_entry.peer_id) {
  //	  peer_match = i;
  //	}
  //      }
  //  
  //      // If the peer is currently in use this rpc is blocked 
  //      if (peer_match != -1) {
  //	new_entry.priority = BLOCKED;
  //      } else {
  //	new_entry.priority = ACTIVE;
  //      }
  //    } 
  //  }
  //
  //  if (!srpt_queue_insert.empty()) {
  //    srpt_grant_entry_t new_entry;
  //    srpt_queue_insert.read(new_entry);
  //  
  //    int peer_match = -1;
  //  
  //    // Is this peer in use?
  //    for (int i = 0; i < MAX_OVERCOMMIT; ++i) {
  //#pragma HLS unroll
  //      if (active_set[i].peer_id == new_entry.peer_id) {
  //	  peer_match = i;
  //      }
  //    }
  //  
  //    // If the peer is currently in use this rpc is blocked 
  //    if (peer_match != -1) {
  //      new_entry.priority = BLOCKED;
  //    } else {
  //      new_entry.priority = ACTIVE;
  //    }
  //  
  //    //      if (active_set[peer_match].rpc_id == new_entry.rpc_id) {
  //    //	active_set_stack.push(peer_match);
  //    //
  //    //	// This receipt indicates the message was fully granted
  //    //	if (new_entry.granted == 0) {
  //    //	  new_entry.priority = MSG;
  //    //	} else {
  //    //	  new_entry.priority = ACTIVE;
  //    //	}
  //    //
  //    //	srpt_queue.push(new_entry);
  //    //      } else {
  //    //	// This is fine because the receipt will unblock this!
  //    //	new_entry.priority = BLOCKED;
  //    //	srpt_queue.push(new_entry);
  //    //      }
  //    //    } else {
  //    //      new_entry.priority = ACTIVE;
  //    //      srpt_queue.push(new_entry);
  //    //    }
  //  } else if (!srpt_queue_next.full() && !active_set_stack.empty()) {
  //    srpt_grant_entry_t top;
  //    //if (srpt_queue.pop(top)) {
  //    //      active_set[active_set_stack.pop()] = top;
  //    srpt_queue_next.write(top);
  //    //      top.priority = MSG;
  //    //      srpt_queue.push(top);
  //    //}
  //  } // TODO fix and reintroduce
  
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





  // TODO are there issues if the final pop happens while a grant arrives?
  //if (!srpt_queue_next.full() && !srpt_queue.empty()) {
  //  srpt_xmit_entry_t & head = srpt_queue.head();

  //  uint32_t grant = grants[head.rpc_id];
  //  uint32_t remaining = remainings[head.rpc_id];

  //  grant = (grant - PACKET_SIZE < 0) ? 0 : grant - PACKET_SIZE;

  //  head.remaining -= PACKET_SIZE;
  //  grants[head.rpc_id] = grant;
  //  remainings[head.rpc_id] = head.remaining;

  //  srpt_queue_next.write(head);
  //  if (grant <= 0) srpt_queue.pop();
  //} else if (!srpt_queue_grants.empty()) {
  //  srpt_xmit_entry_t new_grant;

  //  srpt_queue_grants.read(new_grant);

  //  uint32_t grant = grants[new_grant.rpc_id];
  //  uint32_t remaining = remainings[new_grant.rpc_id];

  //  if (grant == 0) {
  //    new_grant.remaining = remaining;
  //    grants[new_grant.rpc_id] = new_grant.granted;
  //    srpt_queue.push(new_grant);
  //  } else {
  //    grants[new_grant.rpc_id] = grant + new_grant.granted;
  //  }
  //} else if (!srpt_queue_insert.empty()) {
  //  srpt_xmit_entry_t new_entry;

  //  srpt_queue_insert.read(new_entry);

  //  uint32_t grant = grants[new_entry.rpc_id];
  //  uint32_t remaining = remainings[new_entry.rpc_id];

  //  grants[new_entry.rpc_id] = new_entry.remaining;
  //  remainings[new_entry.rpc_id] = remaining - PACKET_SIZE;

  //  srpt_queue.push(new_entry);
  //}








  //if (!srpt_queue_next.full() && !srpt_queue.empty()) {
  //  srpt_xmit_entry_t & head = srpt_queue.head();

  //  uint32_t grant = grants[head.rpc_id];
  //  uint32_t remaining = remainings[head.rpc_id];

  //  head.remaining -= PACKET_SIZE;
  //  grants[head.rpc_id] = grant - PACKET_SIZE;
  //  remainings[head.rpc_id] = remaining - PACKET_SIZE;

  //  srpt_queue_next.write(head);
  //  if (grant <= 0) srpt_queue.pop();
  //} else if (!srpt_queue_grants.empty()) {
  //  srpt_xmit_entry_t new_grant;

  //  srpt_queue_grants.read(new_grant);

  //  uint32_t grant = grants[new_grant.rpc_id];
  //  uint32_t remaining = remainings[new_grant.rpc_id];

  //  if (grant == 0) {
  //    new_grant.remaining = remaining;
  //    grants[new_grant.rpc_id] = new_grant.granted;
  //    srpt_queue.push(new_grant);
  //  } else {
  //    grants[new_grant.rpc_id] = grant + new_grant.granted;
  //    //remainings[new_grant.rpc_id] = new_grant.remaining;
  //  }
  //} else if (!srpt_queue_insert.empty()) {
  //  srpt_xmit_entry_t new_entry;

  //  srpt_queue_insert.read(new_entry);

  //  uint32_t grant = grants[new_entry.rpc_id];
  //  uint32_t remaining = remainings[new_entry.rpc_id];

  //  grants[new_entry.rpc_id] = new_entry.remaining;
  //  remainings[new_entry.rpc_id] = remaining - PACKET_SIZE;

  //  srpt_queue.push(new_entry);
  //}


  //static uint32_t grants[MAX_RPCS];
  //static uint32_t remainings[MAX_RPCS];

  // The RAM needs three ports aparently
  //#pragma HLS bind_storage variable=grants type=RAM_1WNR
  //#pragma HLS bind_storage variable=remainings type=RAM_1WNR

  //#pragma HLS dependence variable=grants inter RAW false
  //#pragma HLS dependence variable=remainings inter RAW false

