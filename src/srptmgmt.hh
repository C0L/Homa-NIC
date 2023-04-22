#ifndef SRPTMGMT_H
#define SRPTMGMT_H

struct srpt_queue_t;

#include "homa.hh"

struct srpt_entry_t {
  homa_rpc_id_t rpc_id; 
  int remaining;
};

/**
 * Shortest remaining processing time queue
 */
struct srpt_queue_t {
  srpt_entry_t buffer[MAX_RPCS+2];

  int size;

  srpt_queue_t() {

    for (int id = 0; id <= MAX_RPCS+1; ++id) {
      buffer[id] = {-1, -1};
    }

    size = 0;
  }

  void push(homa_rpc_id_t rpc_id, unsigned int remaining) {
#pragma HLS ARRAY_PARTITION variable=buffer type=complete
    size++;
    buffer[0].rpc_id = rpc_id;
    buffer[0].remaining = remaining;

    for (int id = MAX_RPCS; id > 0; id-=2) {
#pragma HLS unroll
      if (buffer[id-1].remaining < buffer[id-2].remaining) {
	buffer[id] = buffer[id-2];
      } else {
	buffer[id] = buffer[id-1];
	buffer[id-1] = buffer[id-2];
      }
    }
  }

  homa_rpc_id_t pop() {
#pragma HLS ARRAY_PARTITION variable=buffer type=complete
    size--;
    for (int id = 0; id <= MAX_RPCS; id+=2) {
#pragma HLS unroll
      if (buffer[id+1].remaining > buffer[id+2].remaining) {
	buffer[id] = buffer[id+2];
      } else {
	buffer[id] = buffer[id+1];
	buffer[id+1] = buffer[id+2];
      }
    }

    return buffer[0].rpc_id;
  }

  int get_size() {
    return size;
  }

  bool empty() {
    return (size == 0);
  }

};

#endif
