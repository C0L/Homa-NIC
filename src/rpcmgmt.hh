#include "homa.hh"

/**
 * Stack for storing availible RPCs,
 */
struct homa_rpc_stack_t {
  homma_rpc_id_t buffer[MAX_RPCS];
  int size;

  homa_rpc_stack_t() {
    // Each RPC id begins as available 
    for (int id = 0; id < MAX_RPCS; ++id) {
      buffer[id] = id+1;
    }
    size = MAX_RPCS;
  }

  void push(homa_rpc_id_t rpc_id) {
    // A shift register is inferred
    for (int id = MAX_RPCS-1; id > 0; --id) {
      buffer[id] = buffer[id-1];
    }

    buffer[0] = rpc_id;
    size++;
  }

  homa_rpc_id_t pop() {
    homa_rpc_id_t head = buffer[0];
    // A shift register is inferred
    for (int id = 0; id < MAX_RPCS; ++id) {
      buffer[id] = buffer[id+1];
    }

    size--;
    return head;
  }

  int get_size() {
    return size;
  }
};


rpc_id_t homa_rpc_new_client();
