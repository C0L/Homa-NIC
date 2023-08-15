#include "dma.hh"

// void dma(ap_uint<512> * maxi_in,
// 	 hls::stream<srpt_sendq_t> & dma_req_i,
// 	 hls::stream<dbuff_in_t> & dbuff_in_o,
// 	 ap_uint<512> * maxi_out,
// 	 hls::stream<dma_w_req_t> & dma_w_req_i) {
// 
//     static int max_reads = 44;
//     static int max_writes = 0;
// 
//     static int num_reads  = 0;
//     static int num_writes = 0;
//     while (true) {
// 	if (num_reads != max_reads && !dma_req_i.empty() && !dbuff_in_o.full()) {
// 	    std::cerr << "DMA READ REQ\n";
// 	    srpt_sendq_t dma_req;
// 	    dma_req_i.read(dma_req);
//  	    dbuff_in_t dbuff_in;
// 	    std::cerr << "OFFSET: " << (dma_req(SENDQ_OFFSET) / DBUFF_CHUNK_SIZE) << std::endl;
// 
//  	    dbuff_in.data = *(maxi_in + (dma_req(SENDQ_OFFSET) / DBUFF_CHUNK_SIZE));
//  	    dbuff_in.dbuff_id = dma_req(SENDQ_DBUFF_ID);
//  	    dbuff_in.local_id = dma_req(SENDQ_RPC_ID);
//  	    dbuff_in.offset = dma_req(SENDQ_OFFSET);
//  	    dbuff_in.msg_len = dma_req(SENDQ_MSG_LEN);
//  	    // dbuff_in.last = dma_req.last;
//  	    dbuff_in_o.write(dbuff_in);
// 	    num_reads++;
// 	}
// 
// 	if (!dma_w_req_i.empty()) {
// 	    std::cerr << "DMA WRITE\n";
// 	    dma_w_req_t dma_req;
// 	    dma_w_req_i.read(dma_req);
// 	    // TODO this offset is wrong?
// 	    *(maxi_out + dma_req.offset) = dma_req.data;
// 	}
// 
// 	//if (num_writes != max_writes && !dma_w_req_i.empty()) {
// 	//    dma_w_req_t dma_req;
// 	//    dma_w_req_i.read(dma_req);
// 	//    *((integral_t*) ((maxi_out) + dma_req.offset)) = dma_req.data;
// 	//    num_writes++;
// 	//}
// 
// 	if (num_reads == max_reads && num_writes == max_writes) return;
//     }
//     std::cerr << "DROP OUT\n";
// }

/**
 * dma_read() - Queue requests on the MAXI bus for RPC message data
 * @maxi - The MAXI interface connected to DMA space
 * @dma_requests__dbuff - 64B data chunks for storage in data space
 */
#ifdef STEPPED
void dma_read(bool dma_read_en,
	      volatile ap_uint<512> * maxi,
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
	dbuff_in_t dbuff_in;
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
    //else {
    //	dbuff_in_t dbuff_in;
    //	dbuff_in.data = *maxi;
    //	dbuff_in_o.write(dbuff_in);
    //}
#endif
}

/**
 * dma_write() - Queue requests on the MAXI bus for RPC message data
 * @maxi - The MAXI interface connected to DMA space
 * @dma_requests__dbuff - 64B data chunks for storage in data space
 */
#ifdef STEPPED
void dma_write(bool dma_write_en,
	       volatile ap_uint<512> * maxi,
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
       *(maxi + (dma_req.offset / DBUFF_CHUNK_SIZE)) = dma_req.data;

#ifdef STEPPED
   }

   //else {
   //    *((integral_t*) (((char*) maxi))) = 0;
   //}
#endif
}
