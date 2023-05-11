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
  ap_uint<2> priority;

  srpt_xmit_entry_t() {
    rpc_id = 0;
    remaining = 0xFFFFFFFF;
    granted = 0;
    priority = EMPTY;
  }

  srpt_xmit_entry_t(rpc_id_t rpc_id, uint32_t remaining, uint32_t granted, ap_uint<2> priority) {
    this->rpc_id = rpc_id;
    this->remaining = remaining;
    this->granted = granted;
    this->priority = priority;
  }

  bool operator==(const srpt_xmit_entry_t & other) const {
    return (rpc_id == other.rpc_id &&
	    remaining == other.remaining &&
	    granted == other.granted &&
	    priority == other.priority);
  }

  // Ordering operator
  bool operator>(srpt_xmit_entry_t & other) {
    return remaining > other.remaining;
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
    buffer[insert_head] = value;
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

  void push(T new_entry) {
#pragma HLS array_partition variable=buffer type=complete
    //#pragma HLS pipeline 
    size = (size >= MAX_SRPT) ? size : size+1;
    buffer[0] = new_entry;
    for (int id = MAX_SIZE; id > 0; id-=2) {
#pragma HLS unroll
      if (buffer[id-2] > buffer[id-1]) {
	buffer[id] = buffer[id-2];
      } else {
	buffer[id] = buffer[id-1];
	buffer[id-1] = buffer[id-2];
      }
    }
  }

  //bool pop(T & entry) {
  void pop() {
#pragma HLS array_partition variable=buffer type=complete
    for (int id = 0; id <= MAX_SIZE-2; id+=2) {
#pragma HLS unroll
      if (buffer[id+1] > buffer[id+2]) {
	buffer[id] = buffer[id+2];
      } else {
	buffer[id] = buffer[id+1];
	buffer[id+1] = buffer[id+2];
      }
    }

    size--;
  }

  T & head() {
    return buffer[1];
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
