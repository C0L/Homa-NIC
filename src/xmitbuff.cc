#include "xmitbuff.hh"
#include "rpcmgmt.hh"


void update_xmit_stack(hls::stream<xmit_id_t> & xmit_stack_next,
		       hls::stream<xmit_id_t> & xmit_stack_free) {
  static stack_t<xmit_id_t, NUM_XMIT_BUFFER> xmit_stack(true);
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

void update_xmit_buffer(hls::stream<xmit_in_t> & xmit_buffer_insert,
			hls::stream<pkt_block_t> & pkt_block_in,
			hls::stream<pkt_block_t> & pkt_block_out) {
  static xmit_buffer_t xmit_buffer[NUM_XMIT_BUFFER];
  static ap_uint<14> read_byte[NUM_XMIT_BUFFER];
#pragma HLS array_partition variable=read_byte type=complete

  //#pragma HLS array_partition variable=xmit_buffer type=cyclic factor=32
#pragma HLS pipeline II=1

  xmit_in_t tmp_buffer;
  xmit_req_t tmp_req;
  
  // Do we need to add any data to xmit_buffer
  if (!xmit_buffer_insert.empty()) {
    xmit_buffer_insert.read(tmp_buffer);
    xmit_buffer[tmp_buffer.xmit_id].blocks[tmp_buffer.xmit_offset] = tmp_buffer.xmit_block;
  }

  if (!pkt_block_in.empty()) {
    pkt_block_t pkt_block;
    pkt_block_in.read(pkt_block);
    //pkt_block_t pkt_block = pkt_block_in.read();

    if (pkt_block.type == DATA) {
      if (pkt_block.data_bytes != 0) {
	uint32_t curr_byte = read_byte[pkt_block.xmit_id];
	read_byte[pkt_block.xmit_id] = curr_byte + pkt_block.data_bytes;

	char double_buff[128];
#pragma HLS array_partition variable=double_buff complete

	int block_offset = curr_byte / XMIT_BUFFER_SIZE;
	int subyte_offset = curr_byte - (block_offset * XMIT_BUFFER_SIZE);

	for (int i = 0; i < 64; ++i) {
#pragma HLS unroll
	  double_buff[i] = xmit_buffer[pkt_block.xmit_id].blocks[block_offset].buff[i];
	}

	for (int i = 0; i < 64; ++i) {
#pragma HLS unroll
	  double_buff[i] = xmit_buffer[pkt_block.xmit_id].blocks[block_offset+1].buff[i];
	}

	if (pkt_block.data_bytes == 64) {
	  for (int i = 0; i < 64; ++i) {
#pragma HLS unroll
	    pkt_block.buff[i] = double_buff[subyte_offset + i];
	  }
	} else {
	  for (int i = 0; i < 14; ++i) {
#pragma HLS unroll
	    pkt_block.buff[i + 50] = double_buff[subyte_offset + i];
	  }
	}
      }
    } 

    pkt_block_out.write(pkt_block);
  }
}

