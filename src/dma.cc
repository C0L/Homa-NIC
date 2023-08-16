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

    static int call_count = 0;

    std::cerr << "READ CC " << call_count++ << std::endl;

    srpt_sendq_t dma_req;
#ifdef STEPPED
    if (dma_read_en) {
#endif
	dma_req_i.read(dma_req);
	dbuff_in_t dbuff_in;
	std::cerr << "DMA READ OFFSET " << (dma_req(SENDQ_OFFSET) / DBUFF_CHUNK_SIZE) << std::endl;
	dbuff_in.data = *(maxi + (dma_req(SENDQ_OFFSET) / DBUFF_CHUNK_SIZE));
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
       std::cerr << "DMA WRITE OFFSET " << (dma_req.offset / DBUFF_CHUNK_SIZE) << std::endl;
       *(maxi + (dma_req.offset / DBUFF_CHUNK_SIZE)) = dma_req.data;

#ifdef STEPPED
   }
#endif
}
