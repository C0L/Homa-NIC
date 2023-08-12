#include "dma.hh"

/**
 * dma_read() - Queue requests on the MAXI bus for RPC message data
 * @maxi - The MAXI interface connected to DMA space
 * @dma_requests__dbuff - 64B data chunks for storage in data space
 */
void dma_read(const integral_t * maxi,
	      hls::stream<srpt_sendq_t> & dma_req_i,
	      hls::stream<dbuff_in_t> & dbuff_in_o) {

#pragma HLS pipeline II=1

    srpt_sendq_t dma_req;
    if (dma_req_i.read_nb(dma_req)) {
	std::cerr << "DMA READ\n";
	dbuff_in_t dbuff_in;
	// std::cerr << "OFFSET: " << (dma_req(SENDQ_OFFSET) / DBUFF_CHUNK_SIZE) << std::endl;
	dbuff_in.data = *(maxi + (dma_req(SENDQ_OFFSET) / DBUFF_CHUNK_SIZE));
	dbuff_in.dbuff_id = dma_req(SENDQ_DBUFF_ID);
	dbuff_in.local_id = dma_req(SENDQ_RPC_ID);
	dbuff_in.offset = dma_req(SENDQ_OFFSET);
	dbuff_in.msg_len = dma_req(SENDQ_MSG_LEN);
	// dbuff_in.last = dma_req.last;
	dbuff_in_o.write(dbuff_in);
    }
}

/**
 * dma_write() - Queue requests on the MAXI bus for RPC message data
 * @maxi - The MAXI interface connected to DMA space
 * @dma_requests__dbuff - 64B data chunks for storage in data space
 */
void dma_write(integral_t * maxi,
      hls::stream<dma_w_req_t> & dma_w_req_i) {

#pragma HLS pipeline II=1

   dma_w_req_t dma_req;
   if (dma_w_req_i.read_nb(dma_req)) {
       std::cerr << "DMA WRITE " << dma_req.offset << std::endl;
      // *((integral_t*) (((char*) maxi) + dma_req.offset)) = dma_req.data;
   }
}
