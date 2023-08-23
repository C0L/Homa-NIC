#include "dma.hh"

/**
 * dma_read() - Queue requests on the MAXI bus for RPC message data
 * @maxi - The MAXI interface connected to DMA space
 * @dma_requests__dbuff - 64B data chunks for storage in data space
 */
void dma_read(ap_uint<512> * maxi,
	      hls::stream<srpt_sendq_t> & dma_req_i,
	      hls::stream<dbuff_in_t> & dbuff_in_o) {
#pragma HLS pipeline II=1

    srpt_sendq_t dma_req;
	dma_req_i.read(dma_req);

	ap_uint<8> * byte_offset = (ap_uint<8> *) maxi;

	byte_offset += dma_req(SENDQ_OFFSET);

	dbuff_in_t dbuff_in;
	dbuff_in.data = *((ap_uint<512> *) byte_offset);

	dbuff_in.dbuff_id = dma_req(SENDQ_DBUFF_ID);
	dbuff_in.local_id = dma_req(SENDQ_RPC_ID);
	dbuff_in.offset = dma_req(SENDQ_OFFSET);
	dbuff_in.msg_len = dma_req(SENDQ_MSG_LEN);
	// dbuff_in.last = dma_req.last; // TODO unused
	dbuff_in_o.write(dbuff_in);
}

/**
 * dma_write() - Queue requests on the MAXI bus for RPC message data
 * @maxi - The MAXI interface connected to DMA space
 * @dma_requests__dbuff - 64B data chunks for storage in data space
 */
void dma_write(ap_uint<512> * maxi,
	       hls::stream<dma_w_req_t> & dma_w_req_i) {

#pragma HLS pipeline II=1

   dma_w_req_t dma_req;

       dma_w_req_i.read(dma_req);

       ap_uint<8> * byte_offset = (ap_uint<8> *) maxi;

       byte_offset += dma_req.offset;

       *((ap_uint<512> *) byte_offset) = dma_req.data;
}
