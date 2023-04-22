#ifndef XMITBUFF_H
#define XMITBUFF_H

// Number of buffers to allocate and bits to index it
#define NUM_XMIT_BUFFER 1024
#define NUM_XMIT_BUFFER_INDEX 10

// Index into xmit buffers
typedef ap_uint<NUM_XMIT_BUFFER_INDEX> xmit_id_t;

// Size of transaction unit (bytes) (one message worth of data)
// TODO check this?
#define XMIT_UNIT_SIZE 1500 

struct xmit_unit_t {
  char xmit_unit[XMIT_UNIT_SIZE];
};

// One xmit unit of data
//typedef ap_uint<XMIT_UNIT_SIZE> xmit_unit_t;

// Number of xmit_unit_t per buffer and bits needed to index that
#define XMIT_BUFFER_SIZE 16
#define XMIT_BUFFER_SIZE_INDEX 4

// One full message buffer, which we have NUM_XMIT_BUFFER of and index into
//typedef xmit_unit_t[XMIT_BUFFER_SIZE] xmit_buffer_t;
struct xmit_buffer_t {
  xmit_unit_t xmit_buffer[XMIT_BUFFER_SIZE]; 
};

typedef ap_uint<XMIT_BUFFER_SIZE_INDEX> xmit_offset_t;

struct xmit_req_t {
  xmit_id_t xmit_id;
  xmit_offset_t xmit_offset;
};

struct xmit_in_t {
  xmit_unit_t xmit_unit;
  xmit_id_t xmit_id;
  xmit_offset_t xmit_offset;
};

struct xmit_out_t {
  xmit_unit_t xmit_unit;
};

struct xmit_stack_t {
  xmit_id_t buffer[MAX_RPCS];
  int size;

  xmit_stack_t() {
#pragma HLS ARRAY_PARTITION variable=buffer type=complete
    // Each RPC id begins as available 
    for (int id = 0; id < MAX_RPCS; ++id) {
      buffer[id] = id;
    }
    size = MAX_RPCS;
  }

  void push(xmit_id_t xmit_id) {
#pragma HLS ARRAY_PARTITION variable=buffer type=complete
    /** This must be pipelined for a caller function to be pipelined */
#pragma HLS PIPELINE
    // A shift register is inferred
    for (int id = MAX_RPCS-1; id > 0; --id) {
    #pragma HLS unroll
      buffer[id] = buffer[id-1];
    }

    buffer[0] = xmit_id;
    size++;
  }

  xmit_id_t pop() {
#pragma HLS ARRAY_PARTITION variable=buffer type=complete
    /** This must be pipelined for a caller function to be pipelined */
#pragma HLS PIPELINE
    xmit_id_t head = buffer[0];
    // A shift register is inferred
    for (int id = 0; id < MAX_RPCS; ++id) {
    #pragma HLS unroll
      buffer[id] = buffer[id+1];
    }

    size--;
    return head;
  }

  bool empty() {
    return (size == 0);
  }
};


#endif
