#include "databuff.hh"
#include "stack.hh"
#include "fifo.hh"

/**
 * dbuff_ingress() - Buffer incoming data chunks while waiting for lookup on
 * packet header to determine DMA destination.
 * @chunk_in_o - Incoming data chunks from link destined for DMA
 * @dma_w_req_o - Outgoing requests for data to be placed in DMA
 * @header_in_i - Incoming headers that determine DMA placement
 */
// void msg_spool_ingress(hls::stream<in_chunk_t> & chunk_in_i,
void dbuff_ingress(hls::stream<in_chunk_t> & chunk_in_i,
		       hls::stream<dma_w_req_t> & dma_w_req_o,
		       hls::stream<header_t> & header_in_i,
		       hls::stream<header_t> & header_in_o) {

#pragma HLS pipeline II=1

    static fifo_t<in_chunk_t, 32> rebuff;

    static header_t header_in;
    static bool valid = false;

    if (valid && !rebuff.empty()) {
	in_chunk_t in_chunk = rebuff.remove();

	// Place chunk in DMA space at global offset + packet offset
	dma_w_req_t dma_w_req;

	dma_w_req.offset = in_chunk.offset + ((header_in.ingress_dma_id) * HOMA_MAX_MESSAGE_LENGTH) + header_in.data_offset;
	dma_w_req.data   = in_chunk.buff;

	dma_w_req_o.write(dma_w_req);

	if (in_chunk.last) {
	    valid = false;
	}
    }

    in_chunk_t in_chunk;
    static int num_chunk = 0;
    if (chunk_in_i.read_nb(in_chunk)) {
	rebuff.insert(in_chunk);
    }

    if (!valid && header_in_i.read_nb(header_in)) {
	valid = true;

	if (header_in.packetmap == PMAP_COMPLETE) {
	    header_in_o.write(header_in);
	}
    } 
}

//void msg_cache_ingress(hls::stream<dma_w_req_t> & dma_w_req_i,
//		       hls::stream<dma_w_req_t> & dma_w_req_o) {

// Have an active set of 64 buffers. When data arrives, map the RPC ID to one of those buffers
// How to do this mapping? Search through each of the buffers to find a match? Probably no good.

// TODO where is ID assigned from?

// Have a table that goes from RPC ID -> local buffer

//#pragma HLS pipeline II=1
//
//    static bridges_t bridges[NUM_DBUFF];
//
//#pragma HLS dependence variable=dbuff inter WAR false
//#pragma HLS dependence variable=dbuff inter RAW false
//
//#pragma HLS dependence variable=dbuff intra WAR false
//#pragma HLS dependence variable=dbuff intra RAW false
//
//    // Take input chunks and add them to the data buffer
//    dbuff_in_t dbuff_in;
//    if (dbuff_egress_i.read_nb(dbuff_in)) {
//
//	dbuff_coffset_t chunk_offset = (dbuff_in.offset % (DBUFF_CHUNK_SIZE * DBUFF_NUM_CHUNKS)) / DBUFF_CHUNK_SIZE;
//
//	dbuff[dbuff_in.dbuff_id][chunk_offset] = dbuff_in.data;
//
//	ap_uint<32> dbuffered = dbuff_in.msg_len - MIN((ap_uint<32>) (dbuff_in.offset + (ap_uint<32>) DBUFF_CHUNK_SIZE), dbuff_in.msg_len);
//
//	// TODO fix this
//	// if (dbuff_in.last) {
//	    srpt_dbuff_notif_t dbuff_notif;
//	    dbuff_notif(SRPT_DBUFF_NOTIF_RPC_ID) = dbuff_in.local_id;
//	    dbuff_notif(SRPT_DBUFF_NOTIF_OFFSET) = dbuffered;
//	    dbuff_notif_o.write(dbuff_notif);
//	    // }
//
//    }
//
//    out_chunk_t out_chunk;
//    out_chunk.local_id = 0;
//    // Do we need to process any packet chunks?
//    if (out_chunk_i.read_nb(out_chunk)) {
//	// Is this a data type packet?
//	if (out_chunk.type == DATA && out_chunk.data_bytes != NO_DATA) {
//	    dbuff_coffset_t chunk_offset = (out_chunk.offset % (DBUFF_CHUNK_SIZE * DBUFF_NUM_CHUNKS)) / DBUFF_CHUNK_SIZE;
//	    dbuff_boffset_t byte_offset  = out_chunk.offset % DBUFF_CHUNK_SIZE;
//
//	    ap_uint<1024> double_buff = (dbuff[out_chunk.dbuff_id][chunk_offset+1], dbuff[out_chunk.dbuff_id][chunk_offset]);
//
//#pragma HLS array_partition variable=double_buff complete
//
//	    // TODO would be nice to reduce what is needed here
//
//	    switch(out_chunk.data_bytes) {
//		case ALL_DATA: {
//		    out_chunk.buff(511, 0) = double_buff(((byte_offset + ALL_DATA) * 8)-1, byte_offset * 8);
//		    break;
//		}
//
//		case PARTIAL_DATA: {
//		    out_chunk.buff(511, 512-PARTIAL_DATA*8) = double_buff(((byte_offset + PARTIAL_DATA) * 8)-1, byte_offset * 8);
//		    break;
//		}
//	    }
//	}
//	out_chunk_o.write(out_chunk);
//    }
//}

/**
 * () - Augment outgoing packet chunks with packet data
 * @dbuff_egress_i - Input stream of data that needs to be inserted into the on-chip
 * buffer. Contains the block index of the insertion, and the raw data.
 * @dbuff_notif_o  - Notification to the SRPT core that data is ready on-chip
 * @out_chunk_i    - Input stream for outgoing packets that need to be
 * augmented with data from the data buffer. Each chunk indicates the number of
 * bytes it will need to accumulate from the data buffer, which data buffer,
 * and what global byte offset in the message to send.
 * @link_egress    - Outgoing path to link. 64 byte chunks are placed on the link
 * until the final chunk in a packet is placed on the link with the "last" bit
 * set, indicating a completiton of packet transmission.
 */
// void msg_cache_egress(hls::stream<dbuff_in_t> & dbuff_egress_i,
void msg_cache(hls::stream<dbuff_in_t> & dbuff_egress_i,
		      hls::stream<srpt_dbuff_notif_t> & dbuff_notif_o,
		      hls::stream<out_chunk_t> & out_chunk_i,
		      hls::stream<out_chunk_t> & out_chunk_o) {

#pragma HLS pipeline II=1

    static dbuff_t dbuff[NUM_EGRESS_BUFFS];

#pragma HLS dependence variable=dbuff inter WAR false
#pragma HLS dependence variable=dbuff inter RAW false

#pragma HLS dependence variable=dbuff intra WAR false
#pragma HLS dependence variable=dbuff intra RAW false

    // Take input chunks and add them to the data buffer
    dbuff_in_t dbuff_in;
    if (dbuff_egress_i.read_nb(dbuff_in)) {

	dbuff_coffset_t chunk_offset = (dbuff_in.offset % (DBUFF_CHUNK_SIZE * DBUFF_NUM_CHUNKS)) / DBUFF_CHUNK_SIZE;

	dbuff[dbuff_in.dbuff_id][chunk_offset] = dbuff_in.data;

	ap_uint<32> dbuffered = dbuff_in.msg_len - MIN((ap_uint<32>) (dbuff_in.offset + (ap_uint<32>) DBUFF_CHUNK_SIZE), dbuff_in.msg_len);

	// TODO fix this
	// if (dbuff_in.last) {
	    srpt_dbuff_notif_t dbuff_notif;
	    dbuff_notif(SRPT_DBUFF_NOTIF_RPC_ID) = dbuff_in.local_id;
	    dbuff_notif(SRPT_DBUFF_NOTIF_OFFSET) = dbuffered;
	    dbuff_notif_o.write(dbuff_notif);
	    // }

    }

    out_chunk_t out_chunk;
    out_chunk.local_id = 0;
    // Do we need to process any packet chunks?
    if (out_chunk_i.read_nb(out_chunk)) {
	// Is this a data type packet?
	if (out_chunk.type == DATA && out_chunk.data_bytes != NO_DATA) {
	    dbuff_coffset_t chunk_offset = (out_chunk.offset % (DBUFF_CHUNK_SIZE * DBUFF_NUM_CHUNKS)) / DBUFF_CHUNK_SIZE;
	    dbuff_boffset_t byte_offset  = out_chunk.offset % DBUFF_CHUNK_SIZE;

	    ap_uint<1024> double_buff = (dbuff[out_chunk.egress_buff_id][chunk_offset+1], dbuff[out_chunk.egress_buff_id][chunk_offset]);

#pragma HLS array_partition variable=double_buff complete

	    // TODO would be nice to reduce what is needed here

	    switch(out_chunk.data_bytes) {
		case ALL_DATA: {
		    out_chunk.buff(511, 0) = double_buff(((byte_offset + ALL_DATA) * 8)-1, byte_offset * 8);
		    break;
		}

		case PARTIAL_DATA: {
		    out_chunk.buff(511, 512-PARTIAL_DATA*8) = double_buff(((byte_offset + PARTIAL_DATA) * 8)-1, byte_offset * 8);
		    break;
		}
	    }
	}
	out_chunk_o.write(out_chunk);
    }
}
