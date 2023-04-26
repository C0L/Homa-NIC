#ifndef SRPTMGMT_H
#define SRPTMGMT_H

#include "homa.hh"
#include "rpcmgmt.hh"

#define MAX_SRPT 1024

struct srpt_queue_t;

struct srpt_entry_t {
  rpc_id_t rpc_id; 
  int remaining;
};

/**
 * Shortest remaining processing time queue
 */
struct srpt_queue_t {
  srpt_entry_t buffer[MAX_SRPT+2];

  int size;

  srpt_queue_t() {
    for (int id = 0; id <= MAX_SRPT+1; ++id) {
      buffer[id] = {-1, -1};
    }

    size = 0;
  }

  void push(srpt_entry_t new_entry) {
#pragma HLS ARRAY_PARTITION variable=buffer type=complete
    size++;
    buffer[0] = new_entry;

    for (int id = MAX_SRPT; id > 0; id-=2) {
#pragma HLS unroll
      if (buffer[id-1].remaining < buffer[id-2].remaining) {
	buffer[id] = buffer[id-2];
      } else {
	buffer[id] = buffer[id-1];
	buffer[id-1] = buffer[id-2];
      }
    }
  }

  srpt_entry_t pop() {
#pragma HLS ARRAY_PARTITION variable=buffer type=complete
    size--;
    for (int id = 0; id <= MAX_SRPT; id+=2) {
#pragma HLS unroll
      if (buffer[id+1].remaining > buffer[id+2].remaining) {
	buffer[id] = buffer[id+2];
      } else {
	buffer[id] = buffer[id+1];
	buffer[id+1] = buffer[id+2];
      }
    }

    return buffer[0];
  }

  int get_size() {
    return size;
  }

  bool empty() {
    return (size == 0);
  }

};

void update_srpt_queue(hls::stream<srpt_entry_t> & srpt_queue_insert,
		       hls::stream<srpt_entry_t> & srpt_queue_next);

#endif
