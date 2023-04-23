#include "srptmgmt.hh"

void update_srpt_queue(hls::stream<srpt_entry_t> & srpt_queue_insert,
		       hls::stream<srpt_entry_t> & srpt_queue_next) {
  static srpt_queue_t srpt_queue;

  srpt_entry_t new_entry;
  if (srpt_queue_insert.read_nb(new_entry)) {
    srpt_queue.push(new_entry);
  }

  srpt_entry_t next_rpc;
  if (!srpt_queue_next.full() && !srpt_queue.empty()) {
    next_rpc = srpt_queue.pop();
    srpt_queue_next.write(next_rpc);
  }
}

