#include "databuff.hh"
#include "stack.hh"
#include "fifo.hh"

/**
 * dbuff_ingress() - Buffer incoming data chunks while waiting for lookup on
 * packet header to determine DMA destination.
 * @chunk_in_i - Incoming data chunks from link destined for DMA
 * @dma_w_req_o - Outgoing requests for data to be placed in DMA
 * @header_in_i - Incoming headers that determine DMA placement
 */
void msg_spool_ingress(hls::stream<in_chunk_t> & chunk_in_i,
		       hls::stream<dma_w_req_t> & dma_w_req_o,
		       hls::stream<header_t> & header_in_i,
		       hls::stream<header_t> & header_in_o) {

#pragma HLS pipeline II=1

    static header_t     header_in;       // Header that corresponds to in_chunk_t values
    static ap_uint<32>  pending_write;   // # remaining bytes to DMA corresponding to hdr
    static ap_uint<32>  chunk_offset;    // 

    static ap_uint<1024> buffer;
    static ap_uint<32>   buffer_head;
    static ap_int<32>    buffered;        // # of bytes in the aligned buffer already
    static ap_int<32>    pending_buffer;  // 

    static in_chunk_t chunk_in;

    // TODO naive host memory management
    ap_uint<32> dma_offset = header_in.ingress_dma_id * 16384;

    /* If we have data buffered then let's send it off regardless of
     * how many bytes are buffered. There does not seem to be any
     * downside to sending out an incomplete chunk if everything upstream
     * is fully pipelined.
     */
    if (chunk_in_i.read_nb(chunk_in) || buffered > 0) {
	
	// Buffer more data if we can
	/* The overflow chunk then becomes the start of the next
	 * aligned chunk and the new number of buffered bytes becomes
	 * whatever the overflow was. It is possible there is no
	 * overflow.
	 */
	buffer(1023, (buffer_head * 8)) = chunk_in.data(511, 0);
	buffered += chunk_in.width;
	chunk_in.width = 0;

	pending_buffer -= chunk_in.width;

        /* The number of writable bytes is either limited by distance
	 * to the next alignment or size of the input chunk
	 */
	// ap_int<32> remaining = 64 - buffer_head;
	// ap_int<32> writable_bytes = MIN(remaining, buffered);
	// ap_int<32> writable_bytes = remaining;

	dma_w_req_t dma_w_req;

	/* The write destination of a chunk is determined by:
	 *   1) The DMA offset corresponding to this RPC
	 *   2) The offset of this packet's message data wrt the message
	 *   3) The offset of this chunk within the message
	 */
	dma_w_req.offset = dma_offset + header_in.data_offset + chunk_offset;
	dma_w_req.data   = buffer(511, 0);
	dma_w_req.strobe = MIN((ap_int<32>) (64 - buffer_head), (ap_int<32>) buffered);
	dma_w_req_o.write(dma_w_req);

	buffer(511, 0) = buffer(1023, 511);

	chunk_offset   += MIN((ap_int<32>) (64 - buffer_head), (ap_int<32>) buffered);
	buffered       -= MIN((ap_int<32>) (64 - buffer_head), (ap_int<32>) buffered);

	// pending_write  -= MIN((ap_int<32>) (64 - buffer_head), (ap_int<32>) buffered);
    } else if (pending_buffer == 0 && header_in_i.read_nb(header_in)) { // TODO buffered is wrong test (total buffered == packet size?)
	/* We may read a new header when we are done buffering the
	 * data from the last header (pending_write == 0) and there
	 * is another header for us to grab
	 */
	/* Though the aligned bytes may begin at an offset, for the
	 * purpose of determining how many bytes to WRITE, we begin at
	 * zero bytes buffered. This will ultimately determine the size
	 * of the write performed, while the aligned bytes will only
	 * effect the offset.
	 */
	chunk_offset   = 0;
        buffer_head    = (dma_offset + header_in.data_offset) % 64;
	pending_buffer = header_in.segment_length;

	if ((header_in.packetmap & PMAP_COMP) == PMAP_COMP) {
	    header_in_o.write(header_in);
	}
    }
}

/* The first chunk write is going to potentially begin within an aligned chunk.
 * To account for this, aligned bytes is set which determines
 * how many more bytes we have remaining to write within the
 * next aligned chunk before we DMA it. After the first chunk,
 * all subsequent chunks will have an aligned bytes of 0
 */


/**
 * msg_cache_egress() - Augment outgoing packet chunks with packet data
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
void msg_cache_egress(hls::stream<dbuff_in_t> & dbuff_egress_i,
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
	if (out_chunk.type == DATA && out_chunk.width != 0) {
	    dbuff_coffset_t chunk_offset = (out_chunk.offset % (DBUFF_CHUNK_SIZE * DBUFF_NUM_CHUNKS)) / DBUFF_CHUNK_SIZE;
	    dbuff_boffset_t byte_offset  = out_chunk.offset % DBUFF_CHUNK_SIZE;

	    ap_uint<1024> double_buff = (dbuff[out_chunk.egress_buff_id][chunk_offset+1], dbuff[out_chunk.egress_buff_id][chunk_offset]);

	    // TODO does this do anything???
#pragma HLS array_partition variable=double_buff complete

	    out_chunk.data(511, (512 - (out_chunk.width*8))) = double_buff(((byte_offset + out_chunk.width) * 8) - 1, byte_offset * 8);

	    //switch(out_chunk.width) {
	    //	case ALL_DATA: {
	    //	    out_chunk.data(511, 0) = double_buff(((byte_offset + ALL_DATA) * 8)-1, byte_offset * 8);
	    //	    break;
	    //	}

	    //	case PARTIAL_DATA: {
	    //	    out_chunk.data(511, 512-PARTIAL_DATA*8) = double_buff(((byte_offset + PARTIAL_DATA) * 8)-1, byte_offset * 8);
	    //	    break;
	    //	}
	    //}
	}
	out_chunk_o.write(out_chunk);
    }
}
