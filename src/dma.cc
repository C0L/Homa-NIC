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
#if defined(CSIM) || defined(COSIM)
void dma_read(uint32_t action,
	      ap_uint<512> * maxi,
	      hls::stream<srpt_sendq_t> & dma_req_i,
	      hls::stream<dbuff_in_t> & dbuff_in_o) {
#else 
void dma_read(ap_uint<512> * maxi,
	      hls::stream<srpt_sendq_t> & dma_req_i,
	      hls::stream<dbuff_in_t> & dbuff_in_o) {
#endif 

    srpt_sendq_t dma_req;

#pragma HLS pipeline II=1

#ifdef CSIM
    if (dma_req_i.read_nb(dma_req)) {
	ofstream trace_file;
	trace_file.open(string("../../../../traces/") + string(QUOTE(OFILE)), ios::app);
	trace_file << 4 << std::endl;
	trace_file.close();
#endif

#ifdef COSIM
    if (action == 4) {
	dma_req_i.read(dma_req);
#endif

#ifdef SYNTH
    if (dma_req_i.read_nb(dma_req)) {
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
#if defined(CSIM) || defined(COSIM)
void dma_write(uint32_t action, ap_uint<512> * maxi,
	       hls::stream<dma_w_req_t> & dma_w_req_i) {
#else
void dma_write(ap_uint<512> * maxi,
	       hls::stream<dma_w_req_t> & dma_w_req_i) {
#endif   
    
#pragma HLS pipeline II=1

   dma_w_req_t dma_req;

#ifdef CSIM
   if (dma_w_req_i.read_nb(dma_req)) {
	ofstream trace_file;
	trace_file.open(string("../../../../traces/") + string(QUOTE(OFILE)), ios::app);
	trace_file << 5 << std::endl;
	trace_file.close();
#endif

#ifdef COSIM
    if (action == 5) {
       dma_w_req_i.read(dma_req);
#endif

#ifdef SYNTH 
   if (dma_w_req_i.read_nb(dma_req)) {
#endif
       ap_uint<8> * offset = (ap_uint<8> *) maxi;
       offset += dma_req.offset;

#define AXI_WRITE(strobe) case strobe: { *(((ap_uint<strobe*8> *) offset)) = dma_req.data; break; }

       // This is ugly but I cannot find a better way to configure WSTRB in AXI4
       switch (dma_req.strobe) {
	   case 2: { *(((ap_uint<16*1> *) offset)) = dma_req.data; break; }
	   case 4: { *(((ap_uint<16*2> *) offset)) = dma_req.data; break; }
	   case 6: { *(((ap_uint<16*3> *) offset)) = dma_req.data; break; }
	   case 8: { *(((ap_uint<16*4> *) offset)) = dma_req.data; break; }
	   case 10: { *(((ap_uint<16*5> *) offset)) = dma_req.data; break; }
	   case 12: { *(((ap_uint<16*6> *) offset)) = dma_req.data; break; }
	   case 14: { *(((ap_uint<16*7> *) offset)) = dma_req.data; break; }
	   case 16: { *(((ap_uint<16*8> *) offset)) = dma_req.data; break; }
	   case 18: { *(((ap_uint<16*9> *) offset)) = dma_req.data; break; }
	   case 20: { *(((ap_uint<16*10> *) offset)) = dma_req.data; break; }
	   case 22: { *(((ap_uint<16*11> *) offset)) = dma_req.data; break; }
	   case 24: { *(((ap_uint<16*12> *) offset)) = dma_req.data; break; }
	   case 26: { *(((ap_uint<16*13> *) offset)) = dma_req.data; break; }
	   case 28: { *(((ap_uint<16*14> *) offset)) = dma_req.data; break; }
	   case 30: { *(((ap_uint<16*15> *) offset)) = dma_req.data; break; }
	   case 32: { *(((ap_uint<16*16> *) offset)) = dma_req.data; break; }
	   case 34: { *(((ap_uint<16*17> *) offset)) = dma_req.data; break; }
	   case 36: { *(((ap_uint<16*18> *) offset)) = dma_req.data; break; }
	   case 38: { *(((ap_uint<16*19> *) offset)) = dma_req.data; break; }
	   case 40: { *(((ap_uint<16*20> *) offset)) = dma_req.data; break; }
	   case 42: { *(((ap_uint<16*21> *) offset)) = dma_req.data; break; }
	   case 44: { *(((ap_uint<16*22> *) offset)) = dma_req.data; break; }
	   case 46: { *(((ap_uint<16*23> *) offset)) = dma_req.data; break; }
	   case 48: { *(((ap_uint<16*24> *) offset)) = dma_req.data; break; }
	   case 50: { *(((ap_uint<16*25> *) offset)) = dma_req.data; break; }
	   case 52: { *(((ap_uint<16*26> *) offset)) = dma_req.data; break; }
	   case 54: { *(((ap_uint<16*27> *) offset)) = dma_req.data; break; }
	   case 56: { *(((ap_uint<16*28> *) offset)) = dma_req.data; break; }
	   case 58: { *(((ap_uint<16*29> *) offset)) = dma_req.data; break; }
	   case 60: { *(((ap_uint<16*30> *) offset)) = dma_req.data; break; }
	   case 62: { *(((ap_uint<16*31> *) offset)) = dma_req.data; break; }
	   case 64: { *(((ap_uint<16*32> *) offset)) = dma_req.data; break; }
       }
   }
}
