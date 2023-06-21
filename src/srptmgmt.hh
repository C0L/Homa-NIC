#ifndef SRPTMGMT_H
#define SRPTMGMT_H

#include <iostream>

#include "homa.hh"
#include "rpcmgmt.hh"

template<typename T, int MAX_SIZE>
struct srpt_queue_t;

template<typename T, int MAX_SIZE>
struct srpt_queue_t {
  T buffer[MAX_SIZE+1];
  int size;
  int offset;

  srpt_queue_t() {
    for (int id = 0; id < MAX_SIZE+1; ++id) {
      buffer[id] = T();
    }

    offset = 0;
    size = 0;
  }

  // Case 1
  // id-2 > id-1 < id
  // id-1 < id-2 < id
  //        id-1 < id-2 < id
  
  // Case 2
  // id-2 < id-1 > id
  // id-2 < id   < id-1
  //        id-2 < id < id-1
  
  // Case 3
  // id-2 < id-1 < id
  //        id-2 < id-1 < id
  void push(T new_entry) {
#pragma HLS array_partition variable=buffer type=complete
    size = (size == MAX_SRPT) ? size : size+1;

    T tmp[MAX_SIZE+1];

    for(int id = MAX_SRPT; id > 0; --id) {
#pragma HLS unroll
      buffer[id] = buffer[id-1];
      tmp[id] = buffer[id-1];
    }

    tmp[0] = new_entry;
    buffer[0] = new_entry;

    // TODO MAX_SRPT-1?
    for (int id = 0; id < MAX_SRPT; ++id) {
#pragma HLS unroll
      if (tmp[id] > tmp[id+1]) {
	buffer[id+1] = tmp[id];
	buffer[id] = tmp[id+1];
	buffer[id+1].update_priority(buffer[id]);
      } 
    }
  }

  // Case 1
  //        id   > id+1 < id+2
  //        id+1 < id   < id+2
  // id+1 < id  < id+2
  
  // Case 2
  //        id   < id+1 > id+2
  //        id   < id+2 < id+1
  // id   < id+2 < id+1
  
  // Case 3
  //        id   < id+1 < id+2
  // id   < id+1 < id+2

  void pop() {
    T tmp[MAX_SIZE+1];

    for(int id = 0; id < MAX_SRPT; ++id) {
#pragma HLS unroll
      buffer[id] = buffer[id+1];
      tmp[id] = buffer[id+1];
    }
   
    for (int id = 0; id < MAX_SRPT; ++id) {
#pragma HLS unroll
      if (tmp[id] > tmp[id+1]) {
	buffer[id+1] = tmp[id];
	buffer[id] = tmp[id+1];
	buffer[id+1].update_priority(buffer[id]);
      } 
    }
 
    size--;
  }

    // Case 1
    // id-1 > id   < id+1
    // id   < id-1 < id+1
    
    // Case 2
    // id-1 < id   > id+1
    // id-1 < id+1 < id 

  void order() {
   
#pragma HLS array_partition variable=buffer type=complete

    T tmp[MAX_SIZE+1];

    for(int id = 0; id < MAX_SRPT+1; ++id) {
#pragma HLS unroll
      buffer[id] = buffer[id];
      tmp[id] = buffer[id];
    }
   
    for (int id = 0; id < MAX_SRPT; ++id) {
#pragma HLS unroll
      if (tmp[id] > tmp[id+1]) {
	buffer[id+1] = tmp[id];
	buffer[id] = tmp[id+1];
	buffer[id+1].update_priority(buffer[id]);
      } 
    }
  }

  void dump() {
    std::cerr << std::endl;
    for (int id = 0; id < MAX_SRPT; ++id) {
      std::cerr << id << ":";
      buffer[id].print();
    }
  }
  

  T & head() {
    return buffer[0];
  }

  int get_size() {
    return size;
  }

  bool empty() {
    return (size == 0);
  }
};

void srpt_data_pkts(hls::stream<sendmsg_t> & new_rpc_i,
		    hls::stream<dbuff_notif_t> & data_notif_i,
		    hls::stream<ready_data_pkt_t> & data_pkt_o,
		    hls::stream<header_t> & header_in_i);

void srpt_grant_pkts(hls::stream<header_t> & header_in_i,
		     hls::stream<ready_grant_pkt_t> & grant_pkt_o);

#endif
