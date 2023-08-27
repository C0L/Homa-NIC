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
void msg_spool_ingress(hls::stream<in_chunk_t> & chunk_in_i,
		       hls::stream<dma_w_req_t> & dma_w_req_o,
		       hls::stream<header_t> & header_in_i,
		       hls::stream<header_t> & header_in_o) {

#pragma HLS pipeline II=1

    static header_t     header_in;      // Header that corresponds to in_chunk_t
    static ap_uint<512> aligned_chunk;  // Data block to write to DMA
    static ap_uint<32>  buffered_bytes = 0; // 
    static ap_uint<32>  next_alignment; // How many bytes from next 64 bytes alignment
    static ap_uint<32>  local_offset;   //

    static bool active = false;

    // TODO replace use of all bytes

    // TODO buffered bytes and next alignment are NOT the same
    // TODO alignment refers to the distance from the next 64 byte divide
    // TODO while buffered bytes is how many bytes we have put into the aligned chunk
    // TODO on the first packet these will not be inverses

    // We want the message data to always be in the lowest bytes of the chunk.


    // TODO is this performant enough for keep constant packet ingress??

    // TODO do not like active flag
    if (!header_in_i.empty() && !active && buffered_bytes == 0) {
	std::cerr << "MSG SPOOL IN\n";
	header_in = header_in_i.read();

	active = true;

	// What is the current byte offset within an aligned chunk?
	next_alignment = ALL_DATA - (header_in.data_offset % ALL_DATA);
	buffered_bytes = 0;
	local_offset   = header_in.data_offset;

	aligned_chunk = 0;

	if (header_in.packetmap == PMAP_COMPLETE) {
	    header_in_o.write(header_in);
	}
    }

    // TODO need to write the last chunk!?!?!

    if (!chunk_in_i.empty() && active) {
	std::cerr << "MSG CHUNK IN\n";
    	in_chunk_t in_chunk = chunk_in_i.read();

	// TODO Unify some of this logic?
	// TODO just really need to compute indicies in switch

	switch (in_chunk.type) {
	    case ALL_DATA: {
		std::cerr << "ALL DATA\n";
		aligned_chunk(((buffered_bytes + next_alignment) * 8) - 1, (buffered_bytes * 8)) = in_chunk.buff((next_alignment * 8) - 1, 0);

		buffered_bytes += next_alignment;

		dma_w_req_t dma_w_req;

		// TODO naive buffer management
		ap_uint<32> dma_offset = header_in.ingress_dma_id * HOMA_MAX_MESSAGE_LENGTH;

		dma_w_req.offset = local_offset + dma_offset;
		dma_w_req.data   = aligned_chunk;

		// TODO am going to need to set strobe anyway because initial byte alignment is off
		// TODO no this does not seem like the case for now
		// dma_w_req.strobe = (in_chunk.offset + offset++) % 64;
		    
		dma_w_req_o.write(dma_w_req);

		local_offset += buffered_bytes;

		// How many bytes did we overshoot this last chunk?
		buffered_bytes = ALL_DATA - next_alignment;

		// Did we overshoot this last aligned block?
		if (buffered_bytes != 0) {
		    // TODO load overshoot bytes in
		    aligned_chunk((buffered_bytes * 8) - 1, 0) = in_chunk.buff((ALL_DATA * 8) - 1, (next_alignment * 8));
		}

		next_alignment = ALL_DATA - buffered_bytes;

		break;
	    }
	   
	    case PARTIAL_DATA: {
		std::cerr << "PARTIAL DATA\n";

		std::cerr << next_alignment << std::endl;

		if (next_alignment > PARTIAL_DATA) {
		    std::cerr << "PARTIAL WRITE: " << buffered_bytes << std::endl;
		    aligned_chunk(((buffered_bytes + PARTIAL_DATA) * 8) - 1, buffered_bytes * 8) = in_chunk.buff((PARTIAL_DATA * 8) - 1, 0);

		    buffered_bytes += PARTIAL_DATA;
		    next_alignment -= PARTIAL_DATA;

		    // Local offset does not change because we are still in the same aligned block
		} else {
	
		    aligned_chunk(((buffered_bytes + next_alignment) * 8) - 1, buffered_bytes * 8) = in_chunk.buff((next_alignment * 8) - 1, 0);

		    buffered_bytes += next_alignment;

		    dma_w_req_t dma_w_req;

		    // TODO naive buffer management
		    ap_uint<32> dma_offset = header_in.ingress_dma_id * HOMA_MAX_MESSAGE_LENGTH;

		    dma_w_req.offset = local_offset + dma_offset;
		    dma_w_req.data   = aligned_chunk;

		    dma_w_req_o.write(dma_w_req);

		    local_offset += buffered_bytes;

		    // How many bytes did we overshoot this last chunk?
		    buffered_bytes = PARTIAL_DATA - next_alignment;

		    // Did we overshoot this last aligned block?
		    if (buffered_bytes != 0) {
			// TODO load overshoot bytes in
			aligned_chunk((buffered_bytes * 8) - 1, 0) = in_chunk.buff((PARTIAL_DATA * 8) - 1, next_alignment * 8);
		    }

		    next_alignment = PARTIAL_DATA - buffered_bytes;
		}

		break;
	    }

	}

	if (in_chunk.last) {
	    active = false;
	}
    } else if (buffered_bytes != 0 && !active) {
	
	dma_w_req_t dma_w_req;
	
	// TODO naive buffer management
	ap_uint<32> dma_offset = header_in.ingress_dma_id * HOMA_MAX_MESSAGE_LENGTH;
	
	dma_w_req.offset = local_offset + dma_offset;
	dma_w_req.data   = aligned_chunk;
	
	dma_w_req_o.write(dma_w_req);

	buffered_bytes = 0;
    }
}

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
