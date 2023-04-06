/**
 * Shortest remaining processing time queue
 */
struct srpt_queue_t {
  struct ordered_rpc_t {
    rpc_id_t rpc_id; 
    unsigned int remaining;
  } buffer[MAX_RPCS+2];

  int size;

  srpt_queue_t() {

    for (int id = 0; id <= MAX_RPCS+1; ++id) {
      buffer[id] = {-1, (unsigned int) -1};
    }

    size = 0;
  }

  void push(rpc_id_t rpc_id, unsigned int remaining) {
    size++;
    buffer[0].rpc_id = rpc_id;
    buffer[0].remaining = remaining;

    for (int id = MAX_RPCS; id > 0; id-=2) {
      if (buffer[id-1].remaining < buffer[id-2].remaining) {
	buffer[id] = buffer[id-2];
      } else {
	buffer[id] = buffer[id-1];
	buffer[id-1] = buffer[id-2];
      }
    }
  }

  rpc_id_t pop() {
    size--;

    for (int id = 0; id <= MAX_RPCS; id+=2) {
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
};
