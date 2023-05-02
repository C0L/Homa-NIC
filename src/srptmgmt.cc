#include "srptmgmt.hh"
#include "rpcmgmt.hh"

// One buffer of indicies of max_overcommitment count?

// TODO this is server side SRPT
void update_xmit_srpt_queue(hls::stream<srpt_xmit_entry_t> & srpt_queue_insert,
			    hls::stream<srpt_xmit_entry_t> & srpt_queue_next) {
  static srpt_xmit_queue_t srpt_queue;
#pragma HLS pipeline II=1

  srpt_xmit_entry_t next_rpc;
  srpt_xmit_entry_t new_entry;

  if (!srpt_queue_next.full() && !srpt_queue.empty()) {
    next_rpc = srpt_queue.pop();
    srpt_queue_next.write(next_rpc);
  } else if (srpt_queue_insert.read_nb(new_entry)) {
    srpt_queue.push(new_entry);
  }
}

void update_grant_srpt_queue(hls::stream<srpt_grant_entry_t> & srpt_queue_insert,
			     hls::stream<srpt_grant_entry_t> & srpt_queue_next) {
  static srpt_grant_queue_t srpt_queue;
  static peer_id_t active_set[MAX_OVERCOMMIT];
  static stack_t<peer_id_t, MAX_OVERCOMMIT> active_set_stack(false);

#pragma HLS pipeline II=1

  // The cascading if/else is not being evaluated in parallel
  
  srpt_grant_entry_t new_entry;

  int active_idx = -1;
  // Is the value is in the value in the active set?
  for (int i = 0; i < MAX_OVERCOMMIT; ++i) {
#pragma HLS unroll
    if (active_set[i] == new_entry.peer_id) {
      active_idx = i;
    }
  }


  // TODO need to handle the "update" messages
  // TODO Perform each of these operations each function invocation

  // Is there room in the output FIFO? Have we exceeded max overcommit?
  if (!srpt_queue_next.full() && !active_set_stack.empty()) {
    srpt_grant_entry_t top;
    if (srpt_queue.pop(top)) {
      active_set[active_set_stack.pop()] = top.peer_id;
      srpt_queue_next.write(top);
    }
  // Are there updates to the grant queue?
  } else if (srpt_queue_insert.read_nb(new_entry)) {
    if (active_idx != -1) { 
      active_set_stack.push(active_idx);
      new_entry.priority = 0;
    }

    srpt_queue.push(new_entry);
  // If there is no work to do then just reorder SRPT
  } else {
    srpt_queue.order();
  }
}


