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
void dbuff_ingress(hls::stream<in_chunk_t> & chunk_in_o,
		   hls::stream<dma_w_req_raw_t> & dma_w_req_o,
		   hls::stream<header_t> & header_in_i,
		   hls::stream<header_t> & header_in_o) {

#pragma HLS pipeline II=1

    static fifo_t<in_chunk_t, 16> rebuff;

    // TODO can this be problematic?
    //#pragma HLS dependence variable=rebuff inter WAR false
    //#pragma HLS dependence variable=rebuff inter RAW false

    static header_t header_in;
    static bool valid = false;

    if (valid && !rebuff.empty()) {
	in_chunk_t in_chunk = rebuff.remove();

	// Place chunk in DMA space at global offset + packet offset
	dma_w_req_raw_t dma_w_req;

	dma_w_req(DMA_W_REQ_OFFSET) = in_chunk.offset + ((header_in.ibuff_id) * HOMA_MAX_MESSAGE_LENGTH) + header_in.data_offset;
	dma_w_req(DMA_W_REQ_BLOCK)  = in_chunk.buff;

	dma_w_req_o.write(dma_w_req);

	if (in_chunk.last) valid = false;
    }

    in_chunk_t in_chunk;
    if (!chunk_in_o.empty()) {
	in_chunk = chunk_in_o.read(); 
	rebuff.insert(in_chunk);
    }

    if (!valid && !header_in_i.empty()) {
	valid = true;
	header_in = header_in_i.read();
	header_in_o.write(header_in);
    } 
}

//void dma_req(hls::stream<dma_r_req_raw_t> & sendmsg_reqs_i,
//	     hls::stream<dma_r_req_raw_t> & refresh_reqs_i,
//	     hls::stream<dma_r_req_raw_t> & dma_r_req_o) {
//
//    if (!refresh_reqs_i.empty()) {
//	dma_r_req_raw_t dma_r_req = refresh_reqs_i.read();
//	dma_r_req_o.write(dma_r_req);
//    } else if (!sendmsg_reqs_i.empty()) {
//	dma_r_req_raw_t dma_r_req = sendmsg_reqs_i.read();
//	dma_r_req_o.write(dma_r_req);
//    }
//}

//void onboard_send(hls::stream<onboard_send_t> & onboard_send_i,
//		   hls::stream<onboard_send_t> & onboard_send_o,
//		   hls::stream<dma_r_req_raw_t> & dma_r_req_o) {
//
//    static stack_t<dbuff_id_t, NUM_DBUFF> dbuff_stack(true);
//
//    /* TODO There should be a priority queue here for sendmsg requests
//     * ordered based on the number of bytes left to buffer, bounded by
//     * the max cache size though
//     *
//     * TODO should also stall the onboard message until data is availible
//     */
//    static fifo_t<dma_r_req_raw_t, 128> pending_requests;
//
//    if (!onboard_send_i.empty() && !pending_requests.full()) {
//	onboard_send_t onboard_send = onboard_send_i.read();
//
//	dma_r_req_raw_t dma_r_req;
//    	dma_r_req(DMA_R_REQ_OFFSET)   = onboard_send.iov;
//    	dma_r_req(DMA_R_REQ_BURST)    = MIN((ap_uint<32>) MAX_INIT_CACHE_BURST, onboard_send.iov_size);
//    	dma_r_req(DMA_R_REQ_MSG_LEN)  = onboard_send.iov_size;
//    	dma_r_req(DMA_R_REQ_DBUFF_ID) = onboard_send.dbuff_id;
//    	dma_r_req(DMA_R_REQ_RPC_ID)   = onboard_send.local_id;
//
//    	pending_requests.insert(dma_r_req);
//
//	onboard_send_o.write(onboard_send);
//    }
//
//    if (!rebuffer_i.empty()) {
//	dma_r_req_o.write(rebuffer_i.read());
//    } else if (!pending_requests.empty()){
//	dma_r_req_raw_t & dma_r_req = pending_requests.head();
//	// TODO decrement and check for completion
//	dma_r_req_o.write(dma_r_req);
//
//	dma_r_req(DMA_R_REQ_OFFSET) = dma_r_req(DMA_R_REQ_OFFSET) + DBUFF_CHUNK_SIZE;
//
//	if (dma_r_req(DMA_R_REQ_OFFSET) >= dma_r_req(DMA_R_REQ_BURST)) pending_requests.remove();
//    }
//}

/**
 * dbuff_egress() - Augment outgoing packet chunks with packet data
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
void msg_cache(hls::stream<dbuff_in_raw_t> & dbuff_egress_i,
	       hls::stream<srpt_dbuff_notif_t> & dbuff_notif_o,
	       hls::stream<dma_r_req_raw_t> & sendmsg_req_i,
	       hls::stream<dma_r_req_raw_t> & dma_r_req_o,
	       hls::stream<out_chunk_t> & out_chunk_i,
	       hls::stream<out_chunk_t> & out_chunk_o) {

#pragma HLS pipeline II=1

    // TODO need a way to handle dropped packet events

    static dbuff_t dbuff[NUM_DBUFF];

#pragma HLS bind_storage variable=dbuff type=RAM_1WNR

    // Take input chunks and add them to the data buffer
    dbuff_in_raw_t dbuff_in;
    if (dbuff_egress_i.read_nb(dbuff_in)) {
	dbuff_coffset_t chunk_offset = (dbuff_in(DBUFF_IN_OFFSET) % (DBUFF_CHUNK_SIZE * DBUFF_NUM_CHUNKS)) / DBUFF_CHUNK_SIZE;
	dbuff[dbuff_in(DBUFF_IN_DBUFF_ID)].data[chunk_offset] = dbuff_in(DBUFF_IN_DATA);

	ap_uint<32> dbuffered = dbuff_in(DBUFF_IN_MSG_LEN) - MIN(dbuff_in(DBUFF_IN_OFFSET), dbuff_in(DBUFF_IN_MSG_LEN));

	// TODO THIS IS A PROBLEM!
	if (dbuff_in(DBUFF_IN_LAST)) {
	    srpt_dbuff_notif_t dbuff_notif;
	    dbuff_notif(SRPT_DBUFF_NOTIF_RPC_ID) = dbuff_in(DBUFF_IN_RPC_ID);
	    dbuff_notif(SRPT_DBUFF_NOTIF_OFFSET) = dbuffered;
	    dbuff_notif_o.write(dbuff_notif);
	}
    }

    out_chunk_t out_chunk;
    out_chunk.local_id = 0;
    // Do we need to process any packet chunks?
    if (out_chunk_i.read_nb(out_chunk)) {
	// Is this a data type packet?
	if (out_chunk.type == DATA && out_chunk.data_bytes != NO_DATA) {
	    dbuff_coffset_t chunk_offset = (out_chunk.offset % (DBUFF_CHUNK_SIZE * DBUFF_NUM_CHUNKS)) / DBUFF_CHUNK_SIZE;
	    dbuff_boffset_t byte_offset  = out_chunk.offset % DBUFF_CHUNK_SIZE;

	    ap_uint<1024> double_buff = (dbuff[out_chunk.dbuff_id].data[chunk_offset+1], dbuff[out_chunk.dbuff_id].data[chunk_offset]);

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
   
    if (out_chunk.local_id !=0 && (out_chunk.offset + (DBUFF_CHUNK_SIZE * DBUFF_NUM_CHUNKS)) >= out_chunk.length) {
	dma_r_req_raw_t dma_r_req;
	dma_r_req(DMA_R_REQ_OFFSET)   = out_chunk.offset + DBUFF_CHUNK_SIZE * DBUFF_NUM_CHUNKS;
	dma_r_req(DMA_R_REQ_MSG_LEN)  = out_chunk.length;
	dma_r_req(DMA_R_REQ_DBUFF_ID) = out_chunk.dbuff_id;
	dma_r_req(DMA_R_REQ_RPC_ID)   = out_chunk.local_id;
	// std::cerr << "CHUNK REQ\n";
    } else if (!sendmsg_req_i.empty()) {
	dma_r_req_raw_t dma_r_req = sendmsg_req_i.read();
	dma_r_req_o.write(dma_r_req);
	// std::cerr << "SENDMSG REQ\n";
    }

}
