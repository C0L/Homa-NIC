#include "databuff.hh"
#include "rpcmgmt.hh"

//void update_xmit_stack(hls::stream<onboard_rpc_t> & onboard_rpc_in,
//		       hls::stream<onboard_rpc_t> & onboard_rpc_out,
//		       hls::stream<xmit_id_t> & xmit_stack_free_in) {
//#pragma HLS pipeline II=1
//
//  static stack_t<xmit_id_t, NUM_XMIT_BUFFER> xmit_stack(true);
//
//  if (!onboard_rpc_in.empty()) {
//    onboard_rpc_t onboard_rpc = onboard_rpc_in.read();
//    xmit_id_t next_id = xmit_stack.pop();
//    onboard_rpc.xmit_id = next_id;
//    onboard_rpc_out.write(onboard_rpc);
//  } else if (!xmit_stack_free_in.empty()) {
//    xmit_id_t xmit_id;
//    xmit_stack_free_in.read(xmit_id);
//    xmit_stack.push(freed_id);
//  }
//}

void update_dbuff(hls::stream<dbuff_in_t> & dbuff_i,
		  stream_t<out_block_t, raw_stream_t> & out_pkt_s) {

#pragma HLS pipeline II=1

  static dbuff_t dbuff[NUM_DBUFF];
  static dbuff_boffset_t read_byte[NUM_DBUFF];
  static dbuff_coffset_t write_chunk[NUM_DBUFF];
#pragma HLS array_partition variable=read_byte type=complete
  
  // Do we need to add any data to data buffer 
  if (!dbuff_i.empty()) {
    dbuff_in_t tmp_buffer = dbuff_i.read();
    dbuff_coffset_t chunk_offset = write_chunk[tmp_buffer.dbuff_id];
    chunk_offset++;
    write_chunk[tmp_buffer.dbuff_id] = chunk_offset;
    dbuff[tmp_buffer.dbuff_id][chunk_offset] = tmp_buffer.block;
  }

  if (!out_pkt_s.in.empty()) {
    out_block_t out_block = out_pkt_s.in.read();

    raw_stream_t raw_stream;
    if (out_block.type == DATA && out_block.data_bytes != 0) {
	uint32_t curr_byte = read_byte[out_block.dbuff_id];
	read_byte[out_block.dbuff_id] = curr_byte + out_block.data_bytes;

	char double_buff[128];
#pragma HLS array_partition variable=double_buff complete

	int block_offset = curr_byte / DBUFF_CHUNK_SIZE;
	int subyte_offset = curr_byte - (block_offset * DBUFF_CHUNK_SIZE);

	for (int i = 0; i < 2*DBUFF_CHUNK_SIZE; ++i) {
#pragma HLS unroll
	  double_buff[i] = dbuff[out_block.dbuff_id][block_offset].buff[i];
	  double_buff[i+DBUFF_CHUNK_SIZE] = dbuff[out_block.dbuff_id][block_offset+1].buff[i];
	}

	// TODO this is iffy
	int offset = (out_block.data_bytes == 64) ? 64 : 14;
	for (int i = 0; i < offset; ++i) {
#pragma HLS unroll
	  raw_stream.data[i + 64 - offset] = double_buff[subyte_offset + i];
	}
    } 

    raw_stream.last = out_block.done;

    out_pkt_s.out.write(raw_stream);
  }
}

