#include "dma.hh"

/**
 * dma_read() - Queue requests on the MAXI bus for RPC message data
 * @maxi - The MAXI interface connected to DMA space
 * @dma_requests__dbuff - 64B data chunks for storage in data space
 */
#ifdef STEPPED
void dma_read(bool dma_read_en,
	      ap_uint<512> * maxi,
	      hls::stream<srpt_sendq_t> & dma_req_i,
	      hls::stream<dbuff_in_t> & dbuff_in_o) {
#else
void dma_read(ap_uint<512> * maxi,
	      hls::stream<srpt_sendq_t> & dma_req_i,
	      hls::stream<dbuff_in_t> & dbuff_in_o) {
#endif
    
#pragma HLS pipeline II=1

    srpt_sendq_t dma_req;
#ifdef STEPPED
    if (dma_read_en) {
#endif
	dma_req_i.read(dma_req);

	ap_uint<8> * byte_offset = (ap_uint<8> *) maxi;

	byte_offset += dma_req(SENDQ_OFFSET);

	dbuff_in_t dbuff_in;
	dbuff_in.data = *((ap_uint<512> *) byte_offset);

	std::cerr << "DMA READ " << dbuff_in.data << std::endl;
	dbuff_in.dbuff_id = dma_req(SENDQ_DBUFF_ID);
	dbuff_in.local_id = dma_req(SENDQ_RPC_ID);
	dbuff_in.offset = dma_req(SENDQ_OFFSET);
	dbuff_in.msg_len = dma_req(SENDQ_MSG_LEN);
	// dbuff_in.last = dma_req.last;
	dbuff_in_o.write(dbuff_in);
	// }
#ifdef STEPPED
    }
#endif
}

/**
 * dma_write() - Queue requests on the MAXI bus for RPC message data
 * @maxi - The MAXI interface connected to DMA space
 * @dma_requests__dbuff - 64B data chunks for storage in data space
 */
#ifdef STEPPED
void dma_write(bool dma_write_en,
	       ap_uint<512> * maxi,
	       hls::stream<dma_w_req_t> & dma_w_req_i) {
#else
void dma_write(ap_uint<512> * maxi,
	       hls::stream<dma_w_req_t> & dma_w_req_i) {
#endif

#pragma HLS pipeline II=1

   dma_w_req_t dma_req;

#ifdef STEPPED
   if (dma_write_en) {
#endif
       dma_w_req_i.read(dma_req);

       ap_uint<8> * byte_offset = (ap_uint<8> *) maxi;

       byte_offset += dma_req.offset;

       std::cerr << "WROTE OFFSET " << dma_req.offset << std::endl;
       std::cerr << "WROTE " << dma_req.data << std::endl;

       *((ap_uint<512> *) byte_offset) = dma_req.data;

#ifdef STEPPED
   }
#endif
}
