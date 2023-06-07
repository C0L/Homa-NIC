#include "databuff.hh"
#include "rpcmgmt.hh"
#include <iostream>

// TODO rewrite this using bitslicing and store 512 bit chunks instead?

/**
 * update_dbuff() - Augment outgoing packet chunks with packet data
 * @dbuff_i - Input stream of data that needs to be inserted into the on-chip
 * buffer. Contains the block index of the insertion, and the raw data.
 * @chunk_dispatch__dbuff - Input stream for outgoing packets that need to be
 * augmented with data from the data buffer. Each chunk indicates the number of
 * bytes it will need to accumulate from the data buffer, which data buffer,
 * and what global byte offset in the message to send.
 * @link_egress - Outgoing path to link. 64 byte chunks are placed on the link
 * until the final chunk in a packet is placed on the link with the "last" bit
 * set, indicating a completiton of packet transmission.
 * TODO Needs to request data from DMA to keep the RB saturated with pkt data
 */
void update_dbuff(hls::stream<dbuff_in_t> & dbuff_i,
		  hls::stream<dbuff_notif_t> & dbuff__srpt_data,
		  hls::stream<out_block_t> & chunk_dispatch__dbuff,
		  hls::stream<raw_stream_t> & link_egress) {
	
#pragma HLS pipeline II=1

  // 1024 x 2^14 byte buffers
  static dbuff_t dbuff[NUM_DBUFF];
#pragma HLS bind_storage variable=dbuff type=RAM_1WNR

  // Do we need to add any data to data buffer 
  if (!dbuff_i.empty()) {
    dbuff_in_t dbuff_in = dbuff_i.read();
    DEBUG_MSG("Add Data Chunk: " << dbuff_in.dbuff_chunk << " " << dbuff_in.dbuff_id)
    dbuff[dbuff_in.dbuff_id][dbuff_in.dbuff_chunk] = dbuff_in.block;
    dbuff__srpt_data.write({dbuff_in.dbuff_id, dbuff_in.dbuff_chunk});
  }

  // Do we need to process any packet chunks?
  if (!chunk_dispatch__dbuff.empty()) {
    out_block_t out_block = chunk_dispatch__dbuff.read();

    DEBUG_MSG("Grab Data Chunk: " << out_block.dbuff_id << " " << out_block.offset)

    raw_stream_t raw_stream;

    // Is this a data type packet?
    if (out_block.type == DATA) {
      int curr_byte = out_block.offset % (DBUFF_CHUNK_SIZE * DBUFF_NUM_CHUNKS);

      ap_uint<1024> double_buff;
#pragma HLS array_partition variable=double_buff complete

      int block_offset = curr_byte / DBUFF_CHUNK_SIZE;
      int subyte_offset = curr_byte - (block_offset * DBUFF_CHUNK_SIZE);

      double_buff(511, 0) = dbuff[out_block.dbuff_id][block_offset+1];
      double_buff(1023, 512) = dbuff[out_block.dbuff_id][block_offset];

      raw_stream.data = out_block.buff;

      // There is a more obvious C implementation — results in very expensive hardware 
      switch(out_block.data_bytes) {
      	case NO_DATA: {
      	  break;
      	}
      
      	case ALL_DATA: {
	  raw_stream.data = double_buff(1023 - (subyte_offset * 8), 1024 - ((subyte_offset + ALL_DATA) * 8));
	  break;
      	}
      
      	case PARTIAL_DATA: {
	  std::cerr << "PARTIAL DATA\n";
	  raw_stream.data((PARTIAL_DATA*8)-1, 0) = double_buff(1023 - (subyte_offset * 8), 1024 - ((subyte_offset + PARTIAL_DATA) * 8));
	  break;
      	}
      }
    } 

    raw_stream.last = out_block.last;

    link_egress.write(raw_stream);
  }
}



//void update_dbuff(hls::stream<dbuff_in_t> & dbuff_i,
//		  hls::stream<out_block_t> & chunk_dispatch__dbuff,
//		  hls::stream<raw_stream_t> & link_egress) {
//	
//#pragma HLS pipeline II=1
//
//  // 1024 x 2^14 byte buffers
//  static dbuff_t dbuff[NUM_DBUFF];
//
//  // Do we need to add any data to data buffer 
//  if (!dbuff_i.empty()) {
//    dbuff_in_t dbuff_in = dbuff_i.read();
//    DEBUG_MSG("Add Data Chunk: " << dbuff_in.dbuff_chunk << " " << dbuff_in.dbuff_id)
//
//    dbuff[dbuff_in.dbuff_id].blocks[dbuff_in.dbuff_chunk] = dbuff_in.block;
//  }
//
//  // Do we need to process any packet chunks?
//  if (!chunk_dispatch__dbuff.empty()) {
//    out_block_t out_block = chunk_dispatch__dbuff.read();
//
//    DEBUG_MSG("Grab Data Chunk: " << out_block.dbuff_id << " " << out_block.offset)
//
//    raw_stream_t raw_stream;
//
//    // Is this a data type packet?
//    if (out_block.type == DATA) {
//      int curr_byte = out_block.offset % (DBUFF_CHUNK_SIZE * DBUFF_NUM_CHUNKS);
//
//      char double_buff[128];
//#pragma HLS array_partition variable=double_buff complete
//      
//      int block_offset = curr_byte / DBUFF_CHUNK_SIZE;
//      int subyte_offset = curr_byte - (block_offset * DBUFF_CHUNK_SIZE);
//      
//      for (int i = 0; i < DBUFF_CHUNK_SIZE; ++i) {
//#pragma HLS unroll
//	double_buff[i] = dbuff[out_block.dbuff_id].blocks[block_offset].buff[i];
//	double_buff[i+DBUFF_CHUNK_SIZE] = dbuff[out_block.dbuff_id].blocks[block_offset+1].buff[i];
//      }
//
//      for (int i = 0; i < 64; ++i) {
//#pragma HLS unroll
//	raw_stream.data[i] = out_block.buff[i];
//      }
//
//
//      // There is a more obvious C implementation — results in very expensive hardware 
//      switch(out_block.data_bytes) {
//      	case NO_DATA: {
//      	  break;
//      	}
//      
//      	case ALL_DATA: {
//      	  for (int i = 0; i < ALL_DATA; ++i) {
//#pragma HLS unroll
//      	    raw_stream.data[i] = double_buff[subyte_offset + i];
//      	  }
//      	}
//      
//      	case PARTIAL_DATA: {
//      	  for (int i = 0; i < PARTIAL_DATA; ++i) {
//#pragma HLS unroll
//      	    raw_stream.data[i + 64 - PARTIAL_DATA] = double_buff[subyte_offset + i];
//      	  }
//      	}
//      }
//    } 
//
//    raw_stream.last = out_block.last;
//
//    link_egress.write(raw_stream);
//  }
//}

