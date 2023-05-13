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
#define MAX_SRPT 16
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
  srpt_xmit_entry_t(rpc_id_t rpc_id, uint32_t remaining, uint32_t granted, uint32_t total) { //, ap_uint<2> priority) {
    this->rpc_id = rpc_id;
    this->remaining = remaining;
    this->granted = granted;
    this->total = total;
    //this->priority = priority;
  }

  bool operator==(const srpt_xmit_entry_t & other) const {
    return (rpc_id == other.rpc_id &&
	    remaining == other.remaining &&
	    granted == other.granted &&
	    total == other.total);
	    //priority == other.priority);
  }

  // Ordering operator
  bool operator>(srpt_xmit_entry_t & other) {
    return remaining > other.remaining;
  }


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

  srpt_grant_entry_t(peer_id_t peer_id, rpc_id_t rpc_id, uint32_t grantable, ap_uint<2> priority) {
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
    //if (priority < other.priority) {

    //} else {
    //  reut
    //}
    return grantable > other.grantable;
  }

  void print() {
    std::cerr << peer_id << " " << grantable << " " << priority << std::endl;
  }
};


template<typename T, int FIFO_SIZE>
struct fifo_t {
  T buffer[FIFO_SIZE];

  int insert_head;

  fifo_t() {
    insert_head = 0;
  }

  void insert(T value) {
#pragma HLS array_partition variable=buffer type=complete
    //buffer[insert_head] = value;
    buffer[FIFO_SIZE-1] = value;
    insert_head++;
  }

  void remove(T & value) {
#pragma HLS array_partition variable=buffer type=complete
    value = buffer[0];

    for (int i = 0; i < FIFO_SIZE; ++i) {
#pragma HLS unroll
      buffer[i] = buffer[i+1];
    }

    insert_head--;
  }

  bool empty() {
    return insert_head == 0;
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

    for (int id = MAX_SRPT-1; id >= 2; --id) {
#pragma HLS unroll
      if (buffer[id-2] > buffer[id-1]) {
	buffer[id] = buffer[id-2];
      } else if (buffer[id-1] > buffer[id]) {
	buffer[id] = buffer[id];
      } else {
	buffer[id] = buffer[id-1];
      }
    }

    if (new_entry > buffer[0]) {
      buffer[1] = new_entry;
    } else if (buffer[0] > buffer[1]) {
      buffer[1] = buffer[1];
    } else {
      buffer[1] = buffer[0];
    }

    if (new_entry > buffer[0]) {
      buffer[0] = buffer[0];
    } else {
      buffer[0] = new_entry;
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

    for (int id = 0; id < MAX_SRPT; ++id) {
#pragma HLS unroll
      if (buffer[id] > buffer[id+1]) {
	buffer[id] = buffer[id];
      } else if (buffer[id+1] > buffer[id+2]) {
	buffer[id] = buffer[id+2];
      } else {
	buffer[id] = buffer[id+1];
      }
    }

    if (buffer[MAX_SRPT-1] > buffer[MAX_SRPT]) {
      buffer[MAX_SRPT-1] = buffer[MAX_SRPT-1];
    } else {
      buffer[MAX_SRPT-1] = buffer[MAX_SRPT];
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
    for (int id = 1; id < MAX_SRPT; ++id) {
#pragma HLS unroll
      // If the two entries are out of order
      if (buffer[id-1] > buffer[id]) {
	buffer[id] = buffer[id-1];
      } else if (buffer[id] > buffer[id+1]) {
	buffer[id] = buffer[id+1];
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
			     hls::stream<srpt_grant_entry_t> & srpt_queue_next);


#endif
