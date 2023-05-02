#ifndef SRPTMGMT_H
#define SRPTMGMT_H

#include "homa.hh"
#include "rpcmgmt.hh"

#define MAX_SRPT 1024
#define MAX_OVERCOMMIT 8

struct srpt_queue_t;

struct srpt_xmit_entry_t {
  rpc_id_t rpc_id; 
  uint32_t remaining;

  bool operator==(const srpt_xmit_entry_t & other) const {
    return (rpc_id == other.rpc_id && remaining == other.remaining);
  }
};

/**
 * Shortest remaining processing time queue
 */
struct srpt_xmit_queue_t {
  srpt_xmit_entry_t buffer[MAX_SRPT+2];

  int size;

  srpt_xmit_queue_t() {
    for (int id = 0; id <= MAX_SRPT+1; ++id) {
      buffer[id] = {0xFFFFFFFF, 0xFFFFFFFF};
    }

    size = 0;
  }
  // TODO need to check this logic
  void push(srpt_xmit_entry_t new_entry) {
#pragma HLS array_partition variable=buffer type=complete
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

  srpt_xmit_entry_t pop() {
#pragma HLS array_partition variable=buffer type=complete
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

  bool empty() {
    return (size == 0);
  }
};

struct srpt_grant_entry_t {
  peer_id_t peer_id; 
  uint32_t grantable;
  ap_uint<1> priority;
};

struct srpt_grant_queue_t {
  srpt_grant_entry_t buffer[MAX_SRPT+2];

  int size;

  srpt_grant_queue_t() {
    for (int id = 0; id <= MAX_SRPT+1; ++id) {
      buffer[id] = {0xFFFFFFFF, 0xFFFFFFFF, 0};
    }

    size = 0;
  }

  void push(srpt_grant_entry_t new_entry) {
#pragma HLS array_partition variable=buffer type=complete
    size++;
    buffer[0] = new_entry;

    // Round down to nearest multiple of 2
    for (int id = MAX_SRPT; id > 0; id-=2) {
#pragma HLS unroll
      //if (id > (size & 0xE)) {
      //	continue;
      //}
      // Check if the entry is a "message"
      if (buffer[id-1].peer_id == buffer[id-2].peer_id &&
	  buffer[id-2].grantable == -1) {
	buffer[id-2].priority = 0;
      }
					       
      // If the lower two entries are out of order
      if ((buffer[id-1].grantable < buffer[id-2].grantable) ||
	  (buffer[id-1].priority > buffer[id-2].priority)) {
	// Swap
	buffer[id] = buffer[id-2];
      } else {
	// Shift
	buffer[id] = buffer[id-1];
	buffer[id-1] = buffer[id-2];
      }
    }
  }

  void order() {
#pragma HLS array_partition variable=buffer type=complete
    for (int id = MAX_SRPT; id > 0; id-=2) {
#pragma HLS unroll
      //if (id > (size & 0xE)) {
      //	continue;
      //}

      // Check if the entry is a "message"
      if (buffer[id].peer_id == buffer[id-1].peer_id &&
	  buffer[id-1].grantable == -1) {
	buffer[id-1].priority = 0;
      }
	
      // If the two entries are out of order
      if ((buffer[id].grantable < buffer[id-1].grantable) ||
	  (buffer[id].priority > buffer[id-1].priority)) {
	// Swap
	srpt_grant_entry_t entry = buffer[id];
	buffer[id] = buffer[id-1];
	buffer[id-1] = entry;
      } 
    }
  }

  bool pop(srpt_grant_entry_t & entry) {
#pragma HLS array_partition variable=buffer type=complete

    // Only sucessfully pop if there are RPCs from an ungranted peer
    if (buffer[1].priority == 0) {
      for (int id = 0; id <= MAX_SRPT; id+=2) {
#pragma HLS unroll
	//if (id > (size & 0xE)) {
	//  continue;
	//}
	if (buffer[id+1].peer_id == buffer[id+2].peer_id &&
	    buffer[id+1].grantable == -1) {
	  buffer[id+1].priority = 0;
	}

	if ((buffer[id+2].grantable < buffer[id+1].grantable) ||
	    (buffer[id+2].priority > buffer[id+1].priority)) {
	  buffer[id] = buffer[id+2];
	} else {
	  buffer[id] = buffer[id+1];
	  buffer[id+1] = buffer[id+2];
	}
      }

      entry = buffer[0];
      size--;

      return true;
    } else {
      return false;
    }

    // else {
      //order();
      //return false;
    //}
  }

  bool empty() {
    return (size == 0);
  }
};


void update_xmit_srpt_queue(hls::stream<srpt_xmit_entry_t> & srpt_queue_insert,
			    hls::stream<srpt_xmit_entry_t> & srpt_queue_next);

void update_grant_srpt_queue(hls::stream<srpt_grant_entry_t> & srpt_queue_insert,
			     hls::stream<srpt_grant_entry_t> & srpt_queue_next);


#endif
