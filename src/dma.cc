#include "dma.hh"

/**
 * dma_read() - Queue requests on the MAXI bus for RPC message data
 * @maxi - The MAXI interface connected to DMA space
 * @dma_requests__dbuff - 64B data chunks for storage in data space
 */
void dma_read(const integral_t * maxi,
	      hls::stream<dma_r_req_t> & refresh_req_i,
	      hls::stream<dma_r_req_t> & sendmsg_req_i,
	      hls::stream<dbuff_in_t> & dbuff_in_o) {

#pragma HLS pipeline II=1

    dma_r_req_t dma_req;
    if (refresh_req_i.read_nb(dma_req)) {
	dbuff_in_t dbuff_in;
	dbuff_in.data = *(maxi + (dma_req.offset / DBUFF_CHUNK_SIZE));
	dbuff_in.dbuff_id = dma_req.dbuff_id;
	dbuff_in.local_id = dma_req.local_id;
	dbuff_in.offset = dma_req.offset;
	dbuff_in.msg_len = dma_req.msg_len;
	dbuff_in.last = dma_req.last;
	dbuff_in_o.write(dbuff_in);
    } else if (sendmsg_req_i.read_nb(dma_req)) {
	dbuff_in_t dbuff_in;
	dbuff_in.data = *(maxi + (dma_req.offset / DBUFF_CHUNK_SIZE));
	dbuff_in.dbuff_id = dma_req.dbuff_id;
	dbuff_in.local_id = dma_req.local_id;
	dbuff_in.offset = dma_req.offset;
	dbuff_in.msg_len = dma_req.msg_len;
	dbuff_in.last = dma_req.last;
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
      std::cerr << "OFFSET OUT " << dma_req.offset << std::endl;
      *((integral_t*) (((char*) maxi) + dma_req.offset)) = dma_req.data;
   }
}
