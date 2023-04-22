xmit_buffer_t xmit_buffer[NUM_XMIT_BUFFER];

/*
 * We should be able to process both an incoming xmit unit and outgoing in a single cycle
 * xmit_in need to be written to the message storage
 * xmit_reqs determine what needs to be written to xmit_out
 */
void update_xmit_buffer(hls::stream<xmit_in_t> & xmit_buffer_insert,
			hls::stream<xmit_req_t> & xmit_buffer_request,
			hls::stream<xmit_out_t> & xmit_buffer_response) {
  // TODO invocations need to be pipelined across invocations
  xmit_in_t tmp_buffer;
  xmit_req_t tmp_req;
  
  // Do we need to add any data to xmit_buffer
  if (!xmit_in.empty()) {
    xmit_in.read(tmp_buffer);
    xmit_buffer[tmp_buffer.xmit_id][tmp_buffer.xmit_offset] = tmp_buffer.xmit_unit;
  }

  // Do we have incoming requests and is there space to write the data out?
  if (!xmit_out.full() && !xmit_reqs.empty()) {
    xmit_reqs.read(tmp_req);
    xmit_out.write(xmit_buffer[tmp_req.xmit_id][tmp_req.xmit_offset]);
  }
}


void update_xmit_stack(hls::stream<xmit_id_t> xmit_stack_next,
		       hls::stream<xmit_id_t> xmit_stack_free) {
  static xmit_stack_t xmit_stack;

  xmit_id_t freed_buff;
  if (xmit_stack_free.read_nb(freed_buff)) {
    xmit_stack.push(freed_buff);
  }

  xmit_id_t new_rpc;
  if (!rpc_stack_next.full() && !rpc_stack.empty()) {
    new_rpc = rpc_stack.pop();
    rpc_stack_next.write(new_rpc);
  }
}
