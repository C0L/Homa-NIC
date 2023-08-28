#include "dma.hh"

#include "stdio.h"
#include "string.h"

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

	// TODO revise this indexing? Why need byte indexing when already aligned??

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
       ap_uint<8> * offset = (ap_uint<8> *) maxi;
       offset += dma_req.offset;

#define AXI_WRITE(strobe) case strobe: { *(((ap_uint<strobe*8> *) offset)) = dma_req.data; break; }

       // This is ugly but I cannot find a better way to configure WSTRB in AXI4
       switch (dma_req.strobe) {
	   AXI_WRITE(2)
	   AXI_WRITE(4)
	   AXI_WRITE(6)
	   AXI_WRITE(8)
	   AXI_WRITE(10)
	   AXI_WRITE(12)
	   AXI_WRITE(14)
	   AXI_WRITE(16)
	   AXI_WRITE(18)
	   AXI_WRITE(20)
	   AXI_WRITE(22)
	   AXI_WRITE(24)
	   AXI_WRITE(26)
	   AXI_WRITE(28)
	   AXI_WRITE(30)
	   AXI_WRITE(32)
	   AXI_WRITE(34)
	   AXI_WRITE(36)
	   AXI_WRITE(38)
	   AXI_WRITE(40)
	   AXI_WRITE(42)
	   AXI_WRITE(44)
	   AXI_WRITE(46)
	   AXI_WRITE(48)
	   AXI_WRITE(50)
	   AXI_WRITE(52)
	   AXI_WRITE(54)
	   AXI_WRITE(56)
	   AXI_WRITE(58)
	   AXI_WRITE(60)
	   AXI_WRITE(62)
	   AXI_WRITE(64)
       }
   }
}
