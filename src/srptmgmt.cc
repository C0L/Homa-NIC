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
  // TODO a single byte will grant an entire message

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
    //if (srpt_queue_next.empty() && !srpt_queue.empty()) {
    srpt_xmit_entry_t & head = srpt_queue.head();
    
    srpt_queue_next.write(head);

    head.remaining = (head.remaining - HOMA_PAYLOAD_SIZE < 0) ? 0 : head.remaining - HOMA_PAYLOAD_SIZE;

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
    grants[new_entry.rpc_id] = new_entry.granted;
    srpt_queue.push(new_entry);
  } else if (!srpt_queue_grants.empty()) {
    srpt_xmit_entry_t new_grant;
    srpt_queue_grants.read(new_grant);
    grants[new_grant.rpc_id] = new_grant.granted;
  } else if (!exhausted.empty()) {
    srpt_xmit_entry_t exhausted_entry = exhausted.remove();
    uint32_t grant = grants[exhausted_entry.rpc_id];
    if (grant != exhausted_entry.granted) {
      exhausted_entry.granted = grant;
      srpt_queue.push(exhausted_entry);
    } else {
      exhausted.insert(exhausted_entry);
    }
  } 
}


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
			     hls::stream<srpt_grant_entry_t> & srpt_queue_receipt,
			     hls::stream<srpt_grant_entry_t> & srpt_queue_next) {

  static peer_id_t active_set[MAX_OVERCOMMIT];
  static stack_t<ap_uint<3>, MAX_OVERCOMMIT> active_set_stack(true);
  static srpt_queue_t<srpt_grant_entry_t, MAX_SRPT> srpt_queue;

#pragma HLS pipeline II=1

  // TODO some of these may be receipts
  // TODO Receipt path to reset active set and send message
  if (!srpt_queue_insert.empty()) {
    srpt_grant_entry_t new_entry;
    srpt_queue_insert.read(new_entry);

    int peer_match = -1;
    for (int i = 0; i < MAX_OVERCOMMIT; ++i) {
#pragma HLS unroll
      if (active_set[i] == new_entry.peer_id) {
	peer_match = i;
      }
    }

    if (peer_match != -1) {
      new_entry.priority = BLOCKED;
    } else {
      new_entry.priority = ACTIVE;
    }

    srpt_queue_next.write(new_entry);
  } else if (!srpt_queue_next.full() && srpt_queue.head().priority != BLOCKED && !active_set_stack.empty()) {
    srpt_grant_entry_t & head = srpt_queue.head();

    int peer_match = -1;
    for (int i = 0; i < MAX_OVERCOMMIT; ++i) {
#pragma HLS unroll
      if (active_set[i] == head.peer_id) {
	peer_match = i;
      }
    }

    if (peer_match == -1) {
      srpt_queue_next.write(head);

      ap_uint<3> idx = active_set_stack.pop();
      active_set[idx] = head.peer_id;

      srpt_grant_entry_t msg = srpt_grant_entry_t(head.peer_id, 0, 0, MSG);

      srpt_queue.pop();
      srpt_queue.push(msg);
    }
  } else {
    srpt_queue.order();
  }
}











/* TODO some failed attempt to remove large FIFO reinsert queue
void update_xmit_srpt_queue(hls::stream<srpt_xmit_entry_t> & srpt_queue_insert,
			    hls::stream<srpt_xmit_entry_t> & srpt_queue_grants,
			    hls::stream<srpt_xmit_entry_t> & srpt_queue_next) {
  // TODO can use a smaller FIFO (of pipeline depth) for removing the sync error. After popping from the srpt add it to the blocked RPC BRAM.
  // And add it to the 5 element FIFO. Once it reaches the end of the FIFO, check if the grant has changed, if so readd to SRPT. Otherwise,
  // discard the entry. Need to ensure that a grant does not reactivate and FIFO reactivate.

  static srpt_queue_t<srpt_xmit_entry_t, MAX_SRPT> srpt_queue;
  static uint32_t grants[MAX_RPCS];
  static srpt_xmit_entry_t exhausted[MAX_RPCS];

  // TODO Maybe you can do this without FIFO if the operation to read/write exhausted is atomic? (read and overwrite in a single cycle)
  static fifo_t<srpt_xmit_entry_t, 5> syncfifo; 

    Check if the head of the SRPT is not complete but expended all grantable bytes
    If so, then add the entry to the blocked set in BRAM, and add to FIFO
    Once this entry comes to the head of the FIFO then check again if granted has been updated,
    then also checked if the element is still blocked. If the element is no longer blocked, then that
    means the normal process readded the element. If the element is still blocked, yet the granted value
    is updated, that means the new grant and the removal from the SRPT occured at the same time
    and we should readd to the SRPT

#pragma HLS pipeline II=1

  if (!srpt_queue_next.full() && !srpt_queue.empty()) {
    srpt_xmit_entry_t & head = srpt_queue.head();
    srpt_queue_next.write(head);

    head.remaining = (head.remaining - PACKET_SIZE < 0) ? 0 : head.remaining - PACKET_SIZE;

    uint32_t grant = grants[head.rpc_id];

    // TODO can probably do away with this subtraction step by inverting the value of the grant
    bool ungranted = (head.total - grant == head.remaining);
    bool complete  = (head.remaining == 0);

    if (!complete && ungranted) {
      exhausted[head.rpc_id] = head;
      syncfifo.insert(head);
    }
    
    // Are we done sending this message, or are we out of granted bytes?
    if (complete || ungranted) {
      srpt_queue.pop();
    } 
  } else if (!srpt_queue_insert.empty()) {
    srpt_xmit_entry_t new_entry;
    srpt_queue_insert.read(new_entry);

    srpt_xmit_entry_t exhausted_state = exhausted[new_entry.rpc_id];
    exhausted[new_entry.rpc_id] = srpt_xmit_entry_t();
    grants[new_entry.rpc_id] = new_entry.granted;

    srpt_queue.push(new_entry);
  } else if (!srpt_queue_grants.empty()) {
    srpt_xmit_entry_t new_grant;
    srpt_queue_grants.read(new_grant);

    // Check exhausted BRAM and reset to null value in a single cycle
    srpt_xmit_entry_t exhausted_state = exhausted[new_grant.rpc_id];
    exhausted[new_grant.rpc_id] = srpt_xmit_entry_t();
    grants[new_grant.rpc_id] = new_grant.granted;

    if (exhausted_state.remaining != 0xFFFFFFFF) {
      exhausted_state.granted = new_grant.granted;
      srpt_queue.push(exhausted_state);
    }
  } else if (!syncfifo.empty()) {
    srpt_xmit_entry_t exhausted_entry;
    syncfifo.remove(exhausted_entry);

    srpt_xmit_entry_t exhausted_state = exhausted[exhausted_entry.rpc_id];
    exhausted[exhausted_entry.rpc_id] = srpt_xmit_entry_t();
    uint32_t grant = grants[exhausted_entry.rpc_id];

    if (exhausted_state.remaining != 0xFFFFFFFF) {
      exhausted_entry.granted = grant;
      // TODO add to a secondary insertion FIFO?
      srpt_queue.push(exhausted_entry);
    } 
  }
}

*/
