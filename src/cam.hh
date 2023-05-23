#ifndef CAM_H
#define CAM_H

#include "rpcmgmt.hh"



template<typename K, typename V, int MAX_SIZE>
struct cam_t {
  V buffer[MAX_SIZE];

  int read_head;

  cam_t() {
    read_head = -1;
  }

  bool search(K key, V & value) {
#pragma HLS array_partition variable=buffer type=complete

    bool find = false;
    for (int i = 0; i < MAX_SIZE; ++i) {
#pragma HLS unroll
      if (buffer[i].entry.hashpack == key) {
	value = buffer[i];
	find = true;
	// TODO may eventually need to get all matches and return top one when there is a deletion process
      }
    }

    return find;
  }


  void push(V op) {
#pragma HLS array_partition variable=buffer type=complete
    for (int i = MAX_SIZE-2; i > 0; --i) {
#pragma HLS unroll
      buffer[i+1] = buffer[i];
    }
    
    buffer[0] = op;
    
    read_head++;
  }

  V pop() {
#pragma HLS array_partition variable=buffer type=complete
    V op = buffer[read_head];

    read_head--;
    return op;
  }

  bool empty() {
    return read_head == -1;
  }
};

#endif
