#ifndef CAM_H
#define CAM_H

#include "rpcmgmt.hh"

template<typename K, typename V, int MAX_SIZE>
struct cam_t {
  V buffer[MAX_SIZE];
  int size;

  cam_t() {
    size = 0;
  }

  bool search(K key, V & value) {
#pragma HLS array_partition variable=buffer type=complete
#pragma HLS pipeline
    for (int i = 0; i < MAX_SIZE; ++i) {
#pragma HLS unroll
      // TODO fix type. Need to define structs in here?
      if (buffer[i].entry.hashpack == key) {
	value = buffer[i]; // TODO 
	// TODO may eventually need to get all matches and return top one when there is a deletion process
	return true;
      }
    }
    return false;
  }

  void push(V op) {
#pragma HLS array_partition variable=buffer type=complete
#pragma HLS pipeline
    // A shift register is inferred
    for (int id = MAX_SIZE-1; id > 0; --id) {
#pragma HLS unroll
      buffer[id] = buffer[id-1];
    }

    buffer[0] = op;
    size++;
  }

  V pop() {
#pragma HLS array_partition variable=buffer type=complete
#pragma HLS pipeline
    V head = buffer[0];
    // A shift register is inferred
    for (int id = 0; id < MAX_SIZE; ++id) {
#pragma HLS unroll
      buffer[id] = buffer[id+1];
    }

    size--;
    return head;
  }

  bool empty() {
    return (size == 0);
  }
  // TODO need delete
};

#endif
