#include "databuff.hh"
#include "rpcmgmt.hh"
#include <iostream>

/**
 * dbuff_ingress() - Buffer incoming data chunks while waiting for lookup on
 * packet header to determine DMA destination.
 * @chunk_ingress__dbuff_ingress - Incoming data chunks from link destined for DMA
 * @dbuff_ingress__dma_write - Outgoing requests for data to be placed in DMA
 * @chunk_ingress__dbuff_ingress - Incoming headers that determine DMA placement
 */
void dbuff_ingress(hls::stream<in_chunk_t> & chunk_ingress__dbuff_ingress,
		   hls::stream<dma_w_req_t> & dbuff_ingress__dma_write,
		   hls::stream<header_in_t> & rpc_store__dbuff_ingress) {
#pragma HLS pipeline II=1
  static fifo_t<in_chunk_t,128> rebuff;
  static header_in_t header_in;

  if (!chunk_ingress__dbuff_ingress.empty()) {
    in_chunk_t in_chunk = chunk_ingress__dbuff_ingress.read();
    rebuff.insert(in_chunk);
  }

  if (header_in.valid && !rebuff.empty()) {
    in_chunk_t in_chunk = rebuff.remove();
    // Place chunk in DMA space at global offset + packet offset
    dbuff_ingress__dma_write.write({in_chunk.offset + header_in.dma_offset, in_chunk.buff});

    if (in_chunk.last) header_in.valid = 0;
  }

  if (!header_in.valid && !rpc_store__dbuff_ingress.empty()) {
    header_in = rpc_store__dbuff_ingress.read();
  }
}

/**
 * dbuff_egress() - Augment outgoing packet chunks with packet data
 * @dbuff_egress_i - Input stream of data that needs to be inserted into the on-chip
 * buffer. Contains the block index of the insertion, and the raw data.
 * @chunk_dispatch__dbuff_egress - Input stream for outgoing packets that need to be
 * augmented with data from the data buffer. Each chunk indicates the number of
 * bytes it will need to accumulate from the data buffer, which data buffer,
 * and what global byte offset in the message to send.
 * @link_egress - Outgoing path to link. 64 byte chunks are placed on the link
 * until the final chunk in a packet is placed on the link with the "last" bit
 * set, indicating a completiton of packet transmission.
 * TODO Needs to request data from DMA to keep the RB saturated with pkt data
 */
void dbuff_egress(hls::stream<dbuff_in_t> & dbuff_egress_i,
		  hls::stream<dbuff_notif_t> & dbuff_egress__srpt_data,
		  hls::stream<out_chunk_t> & chunk_dispatch__dbuff_egress,
		  hls::stream<raw_stream_t> & link_egress) {
	
#pragma HLS pipeline II=1

  // 1024 x 2^14 byte buffers
  static dbuff_t dbuff[NUM_DBUFF];
#pragma HLS bind_storage variable=dbuff type=RAM_1WNR

  // Do we need to add any data to data buffer 
  if (!dbuff_egress_i.empty()) {
    dbuff_in_t dbuff_in = dbuff_egress_i.read();
    DEBUG_MSG("Add Data Chunk: " << dbuff_in.dbuff_chunk << " " << dbuff_in.dbuff_id)
    dbuff[dbuff_in.dbuff_id][dbuff_in.dbuff_chunk] = dbuff_in.block;
    dbuff_egress__srpt_data.write({dbuff_in.dbuff_id, dbuff_in.dbuff_chunk});
  }

  // Do we need to process any packet chunks?
  if (!chunk_dispatch__dbuff_egress.empty()) {
    out_chunk_t out_block = chunk_dispatch__dbuff_egress.read();

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
	  raw_stream.data((PARTIAL_DATA*8)-1, 0) = double_buff(1023 - (subyte_offset * 8), 1024 - ((subyte_offset + PARTIAL_DATA) * 8));
	  break;
      	}
      }
    } 

    raw_stream.last = out_block.last;

    link_egress.write(raw_stream);
  }
}
