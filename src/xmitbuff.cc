#include "xmitbuff.hh"
#include "rpcmgmt.hh"

void update_xmit_stack(hls::stream<xmit_id_t> & xmit_stack_next,
		       hls::stream<xmit_id_t> & xmit_stack_free) {
  static stack_t<xmit_id_t, NUM_XMIT_BUFFER> xmit_stack(true);
#pragma HLS pipeline II=1
#pragma HLS pipeline II=1

  xmit_id_t freed_id;

  if (!xmit_stack.empty()) {
    xmit_id_t next_id = xmit_stack.pop();
    xmit_stack_next.write(next_id);
  }

  // TODO expand the rpc_next stream
  if (xmit_stack_free.read_nb(freed_id)) {
    xmit_stack.push(freed_id);
  }
}

/*
 * We should be able to process both an incoming xmit unit and outgoing in a single cycle
 * xmit_in need to be written to the message storage
 * xmit_reqs determine what needs to be written to xmit_out
 */
void update_xmit_buffer(hls::stream<xmit_in_t> & xmit_buffer_insert,
			hls::stream<xmit_req_t> & xmit_buffer_request,
			hls::stream<xmit_mblock_t> & xmit_buffer_response) {
  static xmit_buffer_t xmit_buffer[NUM_XMIT_BUFFER];
  //#pragma HLS array_partition variable=xmit_buffer type=cyclic factor=32
#pragma HLS pipeline II=1

  // TODO invocations need to be pipelined across invocations
  xmit_in_t tmp_buffer;
  xmit_req_t tmp_req;
  
  // Do we need to add any data to xmit_buffer
  if (!xmit_buffer_insert.empty()) {
    xmit_buffer_insert.read(tmp_buffer);
    xmit_buffer[tmp_buffer.xmit_id].buff[tmp_buffer.xmit_offset] = tmp_buffer.xmit_block;
  }

  // Do we have incoming requests and is there space to write the data out?
  if (!xmit_buffer_response.full() && !xmit_buffer_request.empty()) {
    xmit_mblock_t out;
    xmit_buffer_request.read(tmp_req);
    xmit_buffer_response.write(out);
  }
}

