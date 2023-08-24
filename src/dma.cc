#include "dma.hh"

#include <iostream>
#include <fstream>

using namespace std;

/**
 * dma_read() - Queue requests on the MAXI bus for RPC message data
 * @maxi - The MAXI interface connected to DMA space
 * @dma_requests__dbuff - 64B data chunks for storage in data space
 */
void dma_read(uint32_t action,
	      ap_uint<512> * maxi,
	      hls::stream<srpt_sendq_t> & dma_req_i,
	      hls::stream<dbuff_in_t> & dbuff_in_o) {
#pragma HLS pipeline II=1

    srpt_sendq_t dma_req;

#ifdef CSIM
    if (dma_req_i.read_nb(dma_req)) {
	ofstream trace_file;
	trace_file.open("../../../../traces/unscheduled_trace", ios::app);
	trace_file << 4 << std::endl;
	trace_file.close();
#endif

#ifdef COSIM
    if (action == 4) {
	dma_req_i.read(dma_req);
#endif
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
	std::cerr << "DMA READ\n";
    }
}

/**
 * dma_write() - Queue requests on the MAXI bus for RPC message data
 * @maxi - The MAXI interface connected to DMA space
 * @dma_requests__dbuff - 64B data chunks for storage in data space
 */
void dma_write(uint32_t action, ap_uint<512> * maxi,
	       hls::stream<dma_w_req_t> & dma_w_req_i) {

#pragma HLS pipeline II=1

   dma_w_req_t dma_req;

#ifdef CSIM
   if (dma_w_req_i.read_nb(dma_req)) {
	ofstream trace_file;
	trace_file.open("../../../../traces/unscheduled_trace", ios::app);
	trace_file << 5 << std::endl;
	trace_file.close();
#endif

#ifdef COSIM
    if (action == 5) {
       dma_w_req_i.read(dma_req);
#endif

       std::cerr << "OFFSET " << dma_req.offset << std::endl;

       ap_uint<8> * byte_offset = (ap_uint<8> *) maxi;

       byte_offset += dma_req.offset;

       *((ap_uint<512> *) byte_offset) = dma_req.data;
       std::cerr << "DMA WRITE\n";
   }
}
