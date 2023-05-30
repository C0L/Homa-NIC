#ifndef XMITBUFF_H
#define XMITBUFF_H

#include "homa.hh"
#include "net.hh"

#define NUM_XMIT_BUFFER 1024
#define NUM_XMIT_BUFFER_INDEX 10

#define XMIT_UNIT_SIZE 64

//typedef char xmit_mblock_t[XMIT_UNIT_SIZE];
struct xmit_mblock_t {
  char buff[XMIT_UNIT_SIZE];
};

// Number of xmit_unit_t per buffer and bits needed to index that
#define XMIT_BUFFER_SIZE 256
#define XMIT_BUFFER_SIZE_INDEX 8

struct xmit_buffer_t {
  xmit_mblock_t blocks[XMIT_BUFFER_SIZE]; 
};

typedef ap_uint<XMIT_BUFFER_SIZE_INDEX> xmit_offset_t;

struct xmit_req_t {
  xmit_id_t xmit_id;
  xmit_offset_t xmit_offset;
};

struct xmit_in_t {
  xmit_mblock_t xmit_block;
  xmit_id_t xmit_id;
  xmit_offset_t xmit_offset;
};

void update_xmit_buffer(hls::stream<xmit_in_t> & xmit_buffer_insert,
			hls::stream<pkt_block_t> & pkt_block_in,
			hls::stream<pkt_block_t> & pkt_block_out);

void update_xmit_stack(hls::stream<xmit_id_t> & xmit_stack_next,
		       hls::stream<xmit_id_t> & xmit_stack_free);


#endif
