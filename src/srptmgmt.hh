#ifndef SRPTMGMT_H
#define SRPTMGMT_H

#include <iostream>

#include "homa.hh"
#include "rpcmgmt.hh"

// TODO placeholder 
#define PACKET_SIZE 1024

#ifndef DEBUG 
#define MAX_SRPT 1024
#else
#define MAX_SRPT 32
#endif

#define MAX_OVERCOMMIT 8

#define MSG 0
#define EMPTY 1
#define BLOCKED 2
#define ACTIVE 3

template<typename T, int MAX_SIZE>
struct srpt_queue_t;

struct srpt_xmit_entry_t {
  rpc_id_t rpc_id; 
  uint32_t remaining;
  uint32_t granted;
  uint32_t total;

  srpt_xmit_entry_t() {
    rpc_id = 0;
    remaining = 0xFFFFFFFF;
    granted = 0;
    total = 0;
  }

  // TODO total probably does not need to be stored
  srpt_xmit_entry_t(rpc_id_t rpc_id, uint32_t remaining, uint32_t granted, uint32_t total) {
    this->rpc_id = rpc_id;
    this->remaining = remaining;
    this->granted = granted;
    this->total = total;
  }

  bool operator==(const srpt_xmit_entry_t & other) const {
    return (rpc_id == other.rpc_id &&
	    remaining == other.remaining &&
	    granted == other.granted &&
	    total == other.total);
  }

  // Ordering operator
  bool operator>(srpt_xmit_entry_t & other) {
    return remaining > other.remaining;
  }

  void update_priority(srpt_xmit_entry_t & other) {}

  void print() {
    std::cerr << rpc_id << " " << remaining << std::endl;
  }
};

struct srpt_grant_entry_t {
  peer_id_t peer_id;
  rpc_id_t rpc_id;
  uint32_t grantable;
  ap_uint<2> priority;

  srpt_grant_entry_t() {
    peer_id = 0;
    rpc_id = 0;
    grantable = 0xFFFFFFFF;
    priority = EMPTY;
  }

  srpt_grant_entry_t(peer_id_t peer_id, uint32_t grantable,  rpc_id_t rpc_id, ap_uint<2> priority) {
    this->peer_id = peer_id;
    this->rpc_id = rpc_id;
    this->grantable = grantable;
    this->priority = priority;
  }

  bool operator==(const srpt_grant_entry_t & other) const {
    return (peer_id == other.peer_id &&
	    rpc_id == other.rpc_id &&
	    grantable == other.grantable &&
	    priority == other.priority);
  }

  // Ordering operator
  bool operator>(srpt_grant_entry_t & other) {
    if (priority == MSG && peer_id == other.peer_id) {
      return true;
    } else {
      return (priority != other.priority) ? (priority < other.priority) : grantable > other.grantable;
    } 
  }

  void update_priority(srpt_grant_entry_t & other) {
    if (priority == MSG && other.peer_id == peer_id) {
      other.priority = (other.priority == BLOCKED) ? ACTIVE : BLOCKED;
    }
  }

  void print() {
    std::cerr << peer_id << " " << grantable << " " << priority << " " << rpc_id << std::endl;
  }
};


template<typename T, int FIFO_SIZE>
struct fifo_t {
  T buffer[FIFO_SIZE];

  int read_head;

  fifo_t() {
    read_head = -1;
  }

  void insert(T value) {
#pragma HLS array_partition variable=buffer type=complete
    for (int i = FIFO_SIZE-2; i > 0; --i) {
#pragma HLS unroll
      buffer[i+1] = buffer[i];
    }
    
    buffer[0] = value;
    
    read_head++;
  }

  T remove() {
#pragma HLS array_partition variable=buffer type=complete
    T val = buffer[read_head];

    read_head--;
    return val;
  }

  bool empty() {
    return read_head == -1;
  }
};


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
#pragma HLS dependence variable=buffer intra RAW false
#pragma HLS dependence variable=buffer intra WAR false
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
#pragma HLS dependence variable=buffer intra RAW false
#pragma HLS dependence variable=buffer intra WAR false

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
#pragma HLS dependence variable=buffer intra RAW false
#pragma HLS dependence variable=buffer intra WAR false
    
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

void update_xmit_srpt_queue(hls::stream<srpt_xmit_entry_t> & srpt_queue_insert,
			    hls::stream<srpt_xmit_entry_t> & srpt_queue_update,
			    hls::stream<srpt_xmit_entry_t> & srpt_queue_next);

void update_grant_srpt_queue(hls::stream<srpt_grant_entry_t> & srpt_queue_insert,
			     hls::stream<srpt_grant_entry_t> & srpt_queue_receipt,
			     hls::stream<srpt_grant_entry_t> & srpt_queue_next);


#endif
