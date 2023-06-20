#include <iostream>

#include "databuff.hh"
#include "rpcmgmt.hh"
#include "hls_math.h"

void dbuff_stack(hls::stream<sendmsg_t> & sendmsg_i,
		 hls::stream<sendmsg_t> & sendmsg_o,
		 hls::stream<dma_r_req_t> & dma_read_o) {
#pragma HLS pipeline II=1

  static stack_t<dbuff_id_t, NUM_DBUFF> dbuff_stack(true);

  if (!sendmsg_i.empty()) {
    sendmsg_t sendmsg = sendmsg_i.read();

    sendmsg.dbuff_id = dbuff_stack.pop();

    uint32_t num_chunks = sendmsg.length / DBUFF_CHUNK_SIZE;
    if (sendmsg.length % DBUFF_CHUNK_SIZE != 0) num_chunks++;

    dma_read_o.write({sendmsg.buffin, num_chunks, sendmsg.dbuff_id});

    sendmsg_o.write(sendmsg);
  }
}

/**
 * dbuff_ingress() - Buffer incoming data chunks while waiting for lookup on
 * packet header to determine DMA destination.
 * @chunk_in_o - Incoming data chunks from link destined for DMA
 * @dma_w_req_o - Outgoing requests for data to be placed in DMA
 * @header_in_i - Incoming headers that determine DMA placement
 */
void dbuff_ingress(hls::stream<in_chunk_t> & chunk_in_o,
		   hls::stream<dma_w_req_t> & dma_w_req_o,
		   hls::stream<header_t> & header_in_i) {
#pragma HLS pipeline II=1


  static fifo_t<in_chunk_t,16> rebuff;

  // TODO can this be problematic?
  //#pragma HLS dependence variable=rebuff inter WAR false
  //#pragma HLS dependence variable=rebuff inter RAW false

  static header_t header_in;

  if (header_in.valid && !rebuff.empty()) {
    in_chunk_t in_chunk = rebuff.remove();
    // Place chunk in DMA space at global offset + packet offset
    dma_w_req_o.write({in_chunk.offset + header_in.dma_offset + header_in.data_offset, in_chunk.buff});

    if (in_chunk.last) header_in.valid = 0;
  }

  if (!chunk_in_o.empty()) {
    in_chunk_t in_chunk = chunk_in_o.read();
    rebuff.insert(in_chunk);
  }

  if (!header_in.valid && !header_in_i.empty()) {
    header_in = header_in_i.read();
  }
}

/**
 * dbuff_egress() - Augment outgoing packet chunks with packet data
 * @dbuff_egress_i - Input stream of data that needs to be inserted into the on-chip
 * buffer. Contains the block index of the insertion, and the raw data.
 * @dbuff_notif_o - Notification to the SRPT core that data is ready on-chip
 * @out_chunk_i - Input stream for outgoing packets that need to be
 * augmented with data from the data buffer. Each chunk indicates the number of
 * bytes it will need to accumulate from the data buffer, which data buffer,
 * and what global byte offset in the message to send.
 * @link_egress - Outgoing path to link. 64 byte chunks are placed on the link
 * until the final chunk in a packet is placed on the link with the "last" bit
 * set, indicating a completiton of packet transmission.
 * TODO Needs to request data from DMA to keep the RB saturated with pkt data
 */
void dbuff_egress(hls::stream<dbuff_in_t> & dbuff_egress_i,
		  hls::stream<dbuff_notif_t> & dbuff_notif_o,
		  hls::stream<out_chunk_t> & out_chunk_i,
		  hls::stream<raw_stream_t> & link_egress) {
	
#pragma HLS pipeline II=1

  // 1024 x 2^14 byte buffers
  static dbuff_t dbuff[NUM_DBUFF];
#pragma HLS bind_storage variable=dbuff type=RAM_1WNR

  // Do we need to add any data to data buffer 
  if (!dbuff_egress_i.empty()) {
    dbuff_in_t dbuff_in = dbuff_egress_i.read();
    dbuff[dbuff_in.dbuff_id][dbuff_in.dbuff_chunk] = dbuff_in.block;
    dbuff_notif_o.write({dbuff_in.dbuff_id, dbuff_in.dbuff_chunk});
  }

  // Do we need to process any packet chunks?
  if (!out_chunk_i.empty()) {
    out_chunk_t out_block = out_chunk_i.read();

    raw_stream_t raw_stream;

    // Is this a data type packet?
    if (out_block.type == DATA) {
      int curr_byte = out_block.offset % (DBUFF_CHUNK_SIZE * DBUFF_NUM_CHUNKS);

      int block_offset = curr_byte / DBUFF_CHUNK_SIZE;
      int subyte_offset = curr_byte - (block_offset * DBUFF_CHUNK_SIZE);

      integral_t c0 = dbuff[out_block.dbuff_id][block_offset];
      integral_t c1 = dbuff[out_block.dbuff_id][block_offset+1];

      char double_buff[128];

#pragma HLS array_partition variable=double_buff complete
      
      for (int i = 0; i < 64; ++i) {
	double_buff[i]    = c0.data[i];
	double_buff[i+64] = c1.data[i];
      }

      raw_stream.data = out_block.buff;

      // There is a more obvious C implementation — results in very expensive hardware 
     switch(out_block.data_bytes) {
      	case NO_DATA: {
      	  break;
      	}
      
      	case ALL_DATA: {
	  for (int i = 0; i < ALL_DATA; ++i) {
#pragma HLS unroll
	    raw_stream.data.data[i] = double_buff[subyte_offset + i];
	  }
	  break;
      	}
      
      	case PARTIAL_DATA: {
	  for (int i = 0; i < PARTIAL_DATA; ++i) {
#pragma HLS unroll
	    raw_stream.data.data[64 - PARTIAL_DATA + i] = double_buff[subyte_offset + i];
	  }
	  break;
      	}
      }
    } 

    raw_stream.last = out_block.last;

    link_egress.write(raw_stream);
  }
}
