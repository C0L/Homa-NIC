#include "databuff.hh"
#include "stack.hh"
#include "fifo.hh"

/**
 * dbuff_ingress() - When a data packet arrives from the link, we do
 * not know where to place its data in user memory space until the
 * data from its header has been mapped to some local RPC which
 * reveals the write location. While that lookup process is being
 * performed, the data will pile up in the chunk_in_i FIFO. Once the
 * header arrives with host memory information in the header_in_i
 * FIFO, we begin buffering the data chunks into 64 byte aligned
 * chunks, computing the address of their placement and the number of
 * bytes in the write operation, and then sending it to the DMA core.
 * @chunk_in_i  - Incoming data chunks from link destined for DMA
 * @dma_w_req_o - Outgoing requests for data to be placed in DMA
 * @header_in_i - Incoming headers that determine DMA placement
 */
void c2h_databuff(hls::stream<c2h_chunk_t> & chunk_in_i,
		  hls::stream<dma_w_req_t> & dma_w_req_o,
		  hls::stream<header_t> & header_in_i,
		  hls::stream<header_t> & header_in_o) {

#pragma HLS pipeline II=1

    static header_t     header_in;         // Header that corresponds to in_chunk_t values
    static ap_uint<32>  chunk_offset;      // Write offset within the current packet
    static ap_uint<32>  pending_buffer;    // The bytes remaining to buffer from chunk_in_i

    static ap_uint<1024> buffer;           // Data from chunk_in_i written/aligned
    static ap_uint<7>    buffer_head;      // Offset within aligned chunk 
    static ap_uint<7>    buffered;         // Num of bytes in the aligned buffer already

    c2h_chunk_t chunk_in;
    chunk_in.width = 0;

    // TODO naive host memory management
    // ap_uint<32> dma_offset = header_in.ingress_dma_id * 16384;

    ap_uint<64> dma_offset = 0;

    /* There are two conditions in which we interact with the buffer:
     *   1) The value of pending buffer is non-zero which indicates
     *   that we read a header and need to match that header with
     *   pending_buffer number of bytes to be writen out to the
     *   user. This condition indicates that there is data remaining
     *   to buffer but that data must also be availible in chunk_in_i.
     *
     *   2) If we have data buffered then let's send it off regardless
     *   of how many bytes are buffered. There does not seem to be any
     *   downside to sending out an incomplete chunk if everything
     *   upstream is fully pipelined.
     */
    if ((pending_buffer != 0 && chunk_in_i.read_nb(chunk_in)) || buffered > 0) {
	std::cerr << "BUFFERING DATA\n";
	// How many more bytes can we buffer before reaching our next aligned chunk
	ap_uint<7> next_boundary = 64 - buffer_head;

	// Load in the data at the current buffered offset (and potentially overflow it)
	buffer(1023, (buffered * 8)) = chunk_in.data(511, 0);
	pending_buffer -= chunk_in.width;

        /* The number of writable bytes is either limited by distance
	 * to the next alignment or size of the input chunk
	 */
	ap_uint<7> writable_bytes = (chunk_in.width >= next_boundary) ?
	    (ap_uint<7>) (buffered + next_boundary) :
	    (ap_uint<7>) (buffered + chunk_in.width);
        
	dma_w_req_t dma_w_req;

	/* The write destination of a chunk is determined by:
	 *   1) The DMA offset corresponding to this RPC
	 *   2) The offset of this packet's message data wrt the message
	 *   3) The offset of this chunk within the message
	 */
	dma_w_req.offset = dma_offset + header_in.data_offset + chunk_offset;
	dma_w_req.data   = buffer(511, 0);
	dma_w_req.strobe = writable_bytes;
	dma_w_req.port = header_in.sport;
	dma_w_req_o.write(dma_w_req);

	std::cerr << "DMA REQ OFFSET: " << dma_w_req.offset << std::endl;

	buffer >>= (writable_bytes * 8);

	buffer_head = (buffer_head + chunk_in.width) % 64;

	/* If we overflow, the new buffered value is the overflow
	 * bytes, otherwise the buffered value is whatever the previous
	 * buffered value was, plus any new data we just read from an
	 * input chunk, and minus whatever data we just wrote
	 */
	buffered = (chunk_in.width >= next_boundary) ?
	    (ap_uint<7>) (chunk_in.width - next_boundary) :
	    (ap_uint<7>) (buffered + chunk_in.width - writable_bytes);

	chunk_offset += writable_bytes;
    } else if (pending_buffer == 0 && header_in_i.read_nb(header_in)) { 
	/* Though the aligned bytes may begin at an offset, for the
	 * purpose of determining how many bytes to WRITE, we begin at
	 * zero bytes buffered. This will ultimately determine the size
	 * of the write performed, while the aligned bytes will only
	 * effect the offset.
	 */
        buffer_head    = (dma_offset + header_in.data_offset) % 64;
	chunk_offset   = 0;
	pending_buffer = header_in.segment_length;

	if ((header_in.packetmap & PMAP_COMP) == PMAP_COMP) {
	    header_in_o.write(header_in);
	}
    }
}

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
void h2c_databuff(hls::stream<h2c_dbuff_t> & dbuff_egress_i,
		  hls::stream<srpt_queue_entry_t> & dbuff_notif_o,
		  hls::stream<h2c_chunk_t> & h2c_chunk_i,
		  hls::stream<h2c_chunk_t> & h2c_chunk_o,
		  hls::stream<ap_uint<8>> & dbuff_notif_log_o) {

#pragma HLS pipeline II=1

    static dbuff_t dbuff[NUM_EGRESS_BUFFS];

#pragma HLS dependence variable=dbuff inter WAR false
#pragma HLS dependence variable=dbuff inter RAW false

#pragma HLS dependence variable=dbuff intra WAR false
#pragma HLS dependence variable=dbuff intra RAW false

    // Take input chunks and add them to the data buffer
    h2c_dbuff_t dbuff_in;
    if (dbuff_egress_i.read_nb(dbuff_in)) {

	dbuff_coffset_t chunk_offset = (dbuff_in.msg_addr % (DBUFF_CHUNK_SIZE * DBUFF_NUM_CHUNKS)) / DBUFF_CHUNK_SIZE;

	dbuff[dbuff_in.dbuff_id][chunk_offset] = dbuff_in.data;

	// TODO can now remove msg_len from dbuffered
	// ap_uint<32> dbuffered = dbuff_in.msg_len - MIN((ap_uint<32>) (dbuff_in.msg_addr + (ap_uint<32>) DBUFF_CHUNK_SIZE), dbuff_in.msg_len);

	// TODO this is a little overwhelming to the SRPT queue if a notif is sent always
	// if (dbuff_in.last) {
	srpt_queue_entry_t dbuff_notif;
	dbuff_notif(SRPT_QUEUE_ENTRY_RPC_ID)    = dbuff_in.local_id;
	dbuff_notif(SRPT_QUEUE_ENTRY_DBUFFERED) = dbuff_in.msg_addr; // TODO not a misleading name

	dbuff_notif_o.write(dbuff_notif);

	dbuff_notif_log_o.write(LOG_DBUFF_NOTIF);
	    // }
    }

    h2c_chunk_t out_chunk;
    out_chunk.local_id = 0;
    // Do we need to process any packet chunks?
    if (h2c_chunk_i.read_nb(out_chunk)) {
	// Is this a data type packet?
	if (out_chunk.type == DATA && out_chunk.width != 0) {
	    dbuff_coffset_t chunk_offset = (out_chunk.msg_addr % (DBUFF_CHUNK_SIZE * DBUFF_NUM_CHUNKS)) / DBUFF_CHUNK_SIZE;
	    dbuff_boffset_t byte_offset  = out_chunk.msg_addr % DBUFF_CHUNK_SIZE;

	    ap_uint<1024> double_buff = (dbuff[out_chunk.h2c_buff_id][chunk_offset+1], dbuff[out_chunk.h2c_buff_id][chunk_offset]);

	    // TODO does this do anything???
// #pragma HLS array_partition variable=double_buff complete

	    out_chunk.data(511, (512 - (out_chunk.width*8))) = double_buff(((byte_offset + out_chunk.width) * 8) - 1, byte_offset * 8);
	}
	h2c_chunk_o.write(out_chunk);
    }
}
