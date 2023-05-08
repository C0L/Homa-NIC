#include "srptmgmt.hh"
#include "rpcmgmt.hh"

// dma_ingress
//(remaining: 1000000 bytes, granted: 1000, total: 1000000)

// link_egress
// If link egress gets an xmit, where head.total - head.granted == head.remaining
// This means that the message was incompletely granted, and the element is sent to
// link ingress to pend based on further grants

// Could aquire whatever the updated grant value is an go back into the srpt?
// Could just keep cycling around

/**
 * update_xmit_srpt_queue() - Determines what message to transmit next.
 * @srpt_queue_insert: Injest path for new messages to xmit (unscheduled bytes)
 * @srpt_queue_grant: Updates for RPCs when more bytes are granted
 * @srpt_queue_next:   The highest priority value to transmit
 */
void update_xmit_srpt_queue(hls::stream<srpt_xmit_entry_t> & srpt_queue_insert,
			    hls::stream<srpt_xmit_entry_t> & srpt_queue_grants,
			    hls::stream<srpt_xmit_entry_t> & srpt_queue_next) {
  /*
   * Ideally, this is the single source of truth for number of currently granted bytes
   * and remaining number of bytes. Removing global state is really essential.
   *
   * I would like to avoid mutating the state in any way. Would be nice to just add new grants
   * as if they were independent operations
   *
   * What if the link_egress step is responsible for re-adding entries that have become ungranted
   *
   * AVOID MULTIPLE SOURCES OF TRUTH AT ALL COST. THIS IS HOW YOU AVOID SYNCRONIZATION ISSUES.
   *
   * Desirable properties of this style:
   *   1) Only source of truth is the entry which gets passed around
   *   2) No need to synchronize update to some store
   */

  static srpt_queue_t<srpt_xmit_entry_t, MAX_SRPT> srpt_queue;
  static uint32_t grants[MAX_RPCS];
  static fifo_t<srpt_xmit_entry_t, MAX_SRPT> exhausted; // this is really a shift register (allows random read anywhere)

#pragma HLS pipeline II=1

  if (!srpt_queue_next.full() && !srpt_queue.empty()) {
    srpt_xmit_entry_t & head = srpt_queue.head();
      
    head.remaining = (head.remaining - PACKET_SIZE < 0) ? 0 : head.remaining - PACKET_SIZE;
    
    srpt_queue_next.write(head);

    uint32_t grant = grants[head.rpc_id];

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

  static srpt_queue_t<srpt_grant_entry_t, MAX_SRPT> srpt_queue;
  static srpt_grant_entry_t active_set[MAX_OVERCOMMIT];
  static stack_t<peer_id_t, MAX_OVERCOMMIT> active_set_stack(true);

  /*
if the active set has an open slot, we can always pop an element from the srpt queue

the store keeps track of the best results for each active peer id. if a better result arrives,
then add that new result to the SRPT

the srpt just contains the best from each peer

One option, send a MSG up the SRPT queue to clear out the old peer
Then add the new element
Requires two push operations though

how do you remove an element from SRPT?

constantly filter out duplicates?

   */

  //#pragma HLS pipeline II=1

  /* The restrictions are significantly lessened for the grant queue */
  
  if (!srpt_queue_insert.empty()) {
    srpt_grant_entry_t new_entry;
    srpt_queue_insert.read(new_entry);
  
    int peer_match = -1;
  
    // Is this peer in use?
    for (int i = 0; i < MAX_OVERCOMMIT; ++i) {
#pragma HLS unroll
      if (active_set[i].peer_id == new_entry.peer_id) {
	  peer_match = i;
      }
    }
  
    // If the peer is currently in use this rpc is blocked 
    if (peer_match != -1) {
      new_entry.priority = BLOCKED;
    } else {
      new_entry.priority = ACTIVE;
    }
  
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

