#ifndef XMITBUFF_H
#define XMITBUFF_H

#include "homa.hh"

#ifndef DEBUG
// Number of buffers to allocate and bits to index it
#define NUM_XMIT_BUFFER 1024
#define NUM_XMIT_BUFFER_INDEX 10
#else
#define NUM_XMIT_BUFFER 16 
#define NUM_XMIT_BUFFER_INDEX 4
#endif

// Index into xmit buffers
typedef ap_uint<NUM_XMIT_BUFFER_INDEX> xmit_id_t;

// Size of transaction unit (bits) (one message worth of data)
// TODO check this?
//#define XMIT_UNIT_SIZE 1500
#define XMIT_UNIT_SIZE 2048

// typedef char xmit_unit_t[XMIT_UNIT_SIZE];
// TODO try to increase up to 1 packet size?
typedef ap_uint<XMIT_UNIT_SIZE> xmit_mblock_t;

// Number of xmit_unit_t per buffer and bits needed to index that
#define XMIT_BUFFER_SIZE 36
#define XMIT_BUFFER_SIZE_INDEX 4

// One full message buffer, which we have NUM_XMIT_BUFFER of and index into
//typedef xmit_unit_t xmit_buffer_t[XMIT_BUFFER_SIZE];

struct xmit_buffer_t {
  xmit_mblock_t buff[XMIT_BUFFER_SIZE]; 
};

typedef ap_uint<XMIT_BUFFER_SIZE_INDEX> xmit_offset_t;

struct xmit_req_t {
  xmit_id_t xmit_id;
  xmit_offset_t xmit_offset;
};

// TODO This may change if the output size should be larger?
// TODO we need to be able to read an entire packet worth of data in a cycle
struct xmit_in_t {
  xmit_mblock_t xmit_block;
  xmit_id_t xmit_id;
  xmit_offset_t xmit_offset;
};

struct xmit_stack_t {
  xmit_id_t buffer[NUM_XMIT_BUFFER];
  int size;

  xmit_stack_t() {
#pragma HLS ARRAY_PARTITION variable=buffer type=complete
    // Each RPC id begins as available 
    for (int id = 0; id < NUM_XMIT_BUFFER; ++id) {
      buffer[id] = id;
    }
    size = NUM_XMIT_BUFFER;
  }

  void push(xmit_id_t xmit_id) {
#pragma HLS ARRAY_PARTITION variable=buffer type=complete
    /** This must be pipelined for a caller function to be pipelined */
#pragma HLS PIPELINE
    // A shift register is inferred
    for (int id = NUM_XMIT_BUFFER-1; id > 0; --id) {
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
    for (int id = 0; id < NUM_XMIT_BUFFER; ++id) {
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

void update_xmit_buffer(hls::stream<xmit_in_t> & xmit_buffer_insert,
			hls::stream<xmit_req_t> & xmit_buffer_request,
			hls::stream<xmit_mblock_t> & xmit_buffer_response);


void update_xmit_stack(hls::stream<xmit_id_t> & xmit_stack_next,
		       hls::stream<xmit_id_t> & xmit_stack_free);


#endif
