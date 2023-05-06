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
    //this->granted = granted;
    this->priority = priority;
  }

  bool decr() {
    remaining = (remaining - PACKET_SIZE < 0) ? 0 : remaining - PACKET_SIZE;
    //granted = (granted- PACKET_SIZE < 0) ? 0 : granted - PACKET_SIZE;

    return remaining == 0;
    //    return (granted == 0 || remaining == 0);
  }

  bool operator==(const srpt_xmit_entry_t & other) const {
    return (rpc_id == other.rpc_id &&
	    remaining == other.remaining &&
	    //granted == other.granted &&
	    priority == other.priority);
  }

  bool operator==(const bool other) const {
    return true;
  }

  // Ordering operator
  bool operator>(srpt_xmit_entry_t & other) {
    // This is a message and the message is destined for this RPC
    return (remaining > other.remaining);
    //if (priority < other.priority) {
    //  if (rpc_id == other.rpc_id) other.granted += granted;
    //  return true;
    //} else {
    //  return (remaining > other.remaining);
    //}
  }

  void print() {
    std::cerr << rpc_id << " " << remaining << " " << std::endl;
    //std::cerr << rpc_id << " " << remaining << " " << granted << std::endl;
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


  bool decr() {
    return (priority == ACTIVE);
    //if (priority == ACTIVE) {
    //  priority = MSG;
    //}

    //return false; 
  }

  bool operator==(const srpt_grant_entry_t & other) const {
    return (peer_id == other.peer_id &&
	    rpc_id == other.rpc_id &&
	    grantable == other.grantable &&
	    priority == other.priority);
  }

  bool operator==(const bool other) const {
    return (priority == ACTIVE);
  }

  // Ordering operator
  bool operator>(srpt_grant_entry_t & other) {
    if (peer_id == other.peer_id && priority == MSG && other.priority == BLOCKED) {
      other.priority = ACTIVE;
      priority = EMPTY;
      return true;
    } else if (peer_id == other.peer_id && priority == MSG && other.priority == ACTIVE) {
      other.priority = BLOCKED;
      return true;
    } else {
      // Primary ordering based on priority, secondary on grantable bytes
      return (priority != other.priority) ? priority < other.priority : grantable > other.grantable;
    }
  }

  void print() {
    std::cerr << peer_id << " " << grantable << " " << priority << std::endl;
  }
};

template<typename T, int MAX_SIZE>
struct srpt_queue_t {
  T buffer[MAX_SIZE+1];
  int size;

  srpt_queue_t() {
    for (int id = 0; id < MAX_SIZE+1; ++id) {
      buffer[id] = T();
    }

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
    //if (buffer[1].decr()) { 
      for (int id = 0; id <= MAX_SIZE-2; id+=2) {
#pragma HLS unroll
	if (buffer[id+1] > buffer[id+2]) {
	  buffer[id] = buffer[id+2];
	} else {
	  buffer[id] = buffer[id+1];
	  buffer[id+1] = buffer[id+2];
	}
      }
      
      //entry = buffer[0];
      size--;

      //return true;
      //} else {
      //entry = buffer[1];
      //return false;
      //}
  }

  // TODO this is flawed
  void order() {
#pragma HLS array_partition variable=buffer type=complete
    //#pragma HLS pipeline
    for (int id = MAX_SRPT; id > 0; id-=2) {
#pragma HLS unroll
      if (buffer[id-1] > buffer[id]) {
	// Swap
	srpt_grant_entry_t entry = buffer[id];
	buffer[id] = buffer[id-1];
	buffer[id-1] = entry;
      } 
    }
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
